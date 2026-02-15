#include "screensdk/transport/i_webrtc_transport.h"

#include <rtc/rtc.h>
#include <rtc/peerconnection.hpp>
#include <rtc/global.hpp>
#include <rtc/datachannel.hpp>
#include <rtc/track.hpp>

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <algorithm>

namespace screensdk {

// Initialize libdatachannel once at program startup
namespace {
struct LibdatachannelInitializer {
  LibdatachannelInitializer() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    rtc::Preload();
  }
};

static LibdatachannelInitializer g_initializer;
} // anonymous namespace

/**
 * @brief WebRTC transport implementation using libdatachannel
 */
class WebrtcTransportImpl : public IWebrtcTransport {
public:
  WebrtcTransportImpl();
  ~WebrtcTransportImpl() override;

  WebrtcTransportImpl(const WebrtcTransportImpl&) = delete;
  WebrtcTransportImpl& operator=(const WebrtcTransportImpl&) = delete;
  WebrtcTransportImpl(WebrtcTransportImpl&&) = delete;
  WebrtcTransportImpl& operator=(WebrtcTransportImpl&&) = delete;

  bool initialize(const TransportConfig& config) override;
  void shutdown() override;
  Result<std::string> createOffer() override;
  Result<std::string> createAnswer() override;
  Result<void> setRemoteDescription(const std::string& sdp, SdpType type = SdpType::kAnswer) override;
  Result<void> addIceCandidate(const IceCandidate& candidate) override;
  void startVideoTrack(IVideoSource* source) override;
  void stopVideoTrack() override;
  Result<void> sendDataChannelMessage(const std::string& message) override;
  ConnectionState getConnectionState() const noexcept override;
  std::vector<IceCandidate> getLocalIceCandidates() const override;
  void setStateChangeCallback(StateChangeCallback callback) override;
  void setIceCandidateCallback(IceCandidateCallback callback) override;
  void setDataChannelCallback(DataChannelCallback callback) override;
  void setErrorCallback(ErrorCallback callback) override;

private:
  void setupPeerConnectionCallbacks();
  void setupDataChannelCallbacks();
  ConnectionState mapConnectionState(rtc::PeerConnection::State state) const;

  TransportConfig config_;
  std::shared_ptr<rtc::PeerConnection> pc_;
  std::shared_ptr<rtc::DataChannel> dc_;
  std::shared_ptr<rtc::Track> video_track_;
  IVideoSource* video_source_ = nullptr;

  std::atomic<ConnectionState> state_{ConnectionState::kNew};
  std::vector<IceCandidate> local_ice_candidates_;
  mutable std::mutex candidates_mutex_;

  StateChangeCallback state_change_callback_;
  IceCandidateCallback ice_candidate_callback_;
  DataChannelCallback data_channel_callback_;
  ErrorCallback error_callback_;
  mutable std::mutex callback_mutex_;
};

WebrtcTransportImpl::WebrtcTransportImpl() {
  state_.store(ConnectionState::kNew, std::memory_order_release);
}

WebrtcTransportImpl::~WebrtcTransportImpl() {
  shutdown();
}

bool WebrtcTransportImpl::initialize(const TransportConfig& config) {
  if (state_.load(std::memory_order_acquire) != ConnectionState::kNew) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (error_callback_) {
      error_callback_("Already initialized");
    }
    return false;
  }

  config_ = config;

    try {
    rtc::Configuration rtc_config;

    // Configure STUN server if specified
    if (!config.stun_server.empty() && config.stun_server != "") {
      rtc_config.iceServers.emplace_back(config.stun_server);
    }

    // Configure ICE transport
    rtc_config.enableIceTcp = config.enable_ice_tcp;

    // Create PeerConnection
    pc_ = std::make_shared<rtc::PeerConnection>(rtc_config);

    setupPeerConnectionCallbacks();

    state_.store(ConnectionState::kNew, std::memory_order_release);
    return true;

  } catch (const std::exception& e) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (error_callback_) {
      error_callback_(std::string("Failed to initialize: ") + e.what());
    }
    return false;
  }
}

void WebrtcTransportImpl::shutdown() {
  if (pc_) {
    pc_->close();
    pc_.reset();
  }

  if (dc_) {
    dc_->close();
    dc_.reset();
  }

  if (video_track_) {
    video_track_.reset();
  }

  state_.store(ConnectionState::kClosed, std::memory_order_release);
}

Result<std::string> WebrtcTransportImpl::createOffer() {
  if (!pc_) {
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 1001, "PeerConnection not initialized");
  }

  auto current_state = state_.load(std::memory_order_acquire);
  if (current_state == ConnectionState::kClosed ||
      current_state == ConnectionState::kFailed) {
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 1002, "PeerConnection is closed or failed");
  }

  try {
    // Create data channel for control signaling (needed for SDP generation)
    if (!dc_) {
      dc_ = pc_->createDataChannel("control");
      if (dc_) {
        setupDataChannelCallbacks();
      }
    }

    // Create offer synchronously
    rtc::Description desc = pc_->createOffer();
    std::string sdp = desc.generateSdp();
    
    if (sdp.empty()) {
      return Result<std::string>::make_error(
          ErrorType::kNetworkError, 1003, "Failed to generate SDP offer");
    }

    state_.store(ConnectionState::kChecking, std::memory_order_release);
    return Result<std::string>::make_ok(sdp);

  } catch (const std::exception& e) {
    state_.store(ConnectionState::kFailed, std::memory_order_release);
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 1004,
        std::string("Failed to create offer: ") + e.what());
  }
}

Result<std::string> WebrtcTransportImpl::createAnswer() {
  if (!pc_) {
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 1001, "PeerConnection not initialized");
  }

  auto current_state = state_.load(std::memory_order_acquire);
  std::cout << "[WebrtcTransport] createAnswer called, current state: "
            << static_cast<int>(current_state) << std::endl;

  if (current_state == ConnectionState::kClosed ||
      current_state == ConnectionState::kFailed) {
    std::cerr << "[WebrtcTransport] PeerConnection is in invalid state: "
              << static_cast<int>(current_state) << std::endl;
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 1002, "PeerConnection is closed or failed");
  }

  try {
    // Create answer using libdatachannel
    // Note: Do NOT create data channel here. As the answer side, we will receive
    // the data channel via onDataChannel callback after setRemoteDescription is called
    std::cout << "[WebrtcTransport] Calling pc_->createAnswer()..." << std::endl;
    rtc::Description desc = pc_->createAnswer();
    std::string sdp = desc.generateSdp();

    if (sdp.empty()) {
      std::cerr << "[WebrtcTransport] Generated empty SDP answer" << std::endl;
      return Result<std::string>::make_error(
          ErrorType::kNetworkError, 1003, "Failed to generate SDP answer");
    }

    std::cout << "[WebrtcTransport] Created answer successfully, SDP length: "
              << sdp.length() << std::endl;
    state_.store(ConnectionState::kChecking, std::memory_order_release);
    return Result<std::string>::make_ok(sdp);

  } catch (const std::exception& e) {
    state_.store(ConnectionState::kFailed, std::memory_order_release);
    std::cerr << "[WebrtcTransport] Exception in createAnswer: "
              << e.what() << std::endl;
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 1006,
        std::string("Failed to create answer: ") + e.what());
  }
}

Result<void> WebrtcTransportImpl::setRemoteDescription(const std::string& sdp,
                                                        SdpType type) {
  if (!pc_) {
    return Result<void>::make_error(
        ErrorType::kNetworkError, 1001, "PeerConnection not initialized");
  }

  if (sdp.empty()) {
    return Result<void>::make_error(
        ErrorType::kUnknownError, 2001, "SDP string is empty");
  }

  try {
    // Log current state before setting remote description
    auto current_state = state_.load(std::memory_order_acquire);
    std::cout << "[WebrtcTransport] Current state before setRemoteDescription: "
              << static_cast<int>(current_state) << std::endl;

    // Convert SdpType to string for libdatachannel
    std::string type_string = (type == SdpType::kOffer) ? "offer" : "answer";
    std::cout << "[WebrtcTransport] Setting remote description, type: "
              << type_string << ", SDP length: " << sdp.length() << std::endl;

    rtc::Description desc(sdp, type_string);
    pc_->setRemoteDescription(desc);

    // Log state after setting remote description
    current_state = state_.load(std::memory_order_acquire);
    std::cout << "[WebrtcTransport] Current state after setRemoteDescription: "
              << static_cast<int>(current_state) << std::endl;

    return Result<void>::make_ok();

  } catch (const std::exception& e) {
    state_.store(ConnectionState::kFailed, std::memory_order_release);
    std::cerr << "[WebrtcTransport] Exception in setRemoteDescription: "
              << e.what() << std::endl;
    return Result<void>::make_error(
        ErrorType::kNetworkError, 1005,
        std::string("Failed to set remote description: ") + e.what());
  }
}

Result<void> WebrtcTransportImpl::addIceCandidate(const IceCandidate& candidate) {
  if (!pc_) {
    return Result<void>::make_error(
        ErrorType::kNetworkError, 1001, "PeerConnection not initialized");
  }

  if (candidate.candidate.empty()) {
    return Result<void>::make_error(
        ErrorType::kUnknownError, 2002, "ICE candidate is empty");
  }

  try {
    auto current_state = state_.load(std::memory_order_acquire);
    std::cout << "[WebrtcTransport] Adding ICE candidate, current state: "
              << static_cast<int>(current_state) << std::endl;

    rtc::Candidate cand(candidate.candidate, candidate.sdp_mid);
    pc_->addRemoteCandidate(cand);

    std::cout << "[WebrtcTransport] ICE candidate added successfully" << std::endl;
    return Result<void>::make_ok();

  } catch (const std::exception& e) {
    std::cerr << "[WebrtcTransport] Exception in addIceCandidate: "
              << e.what() << std::endl;
    return Result<void>::make_error(
        ErrorType::kNetworkError, 1006,
        std::string("Failed to add ICE candidate: ") + e.what());
  }
}

void WebrtcTransportImpl::startVideoTrack(IVideoSource* source) {
  if (!pc_) {
    return;
  }

  if (video_track_) {
    stopVideoTrack();
  }

  if (!source) {
    return;
  }

  try {
    video_source_ = source;

    // Create video track
    std::string track_id = "video_" + std::to_string(reinterpret_cast<uintptr_t>(source));
    rtc::Description::Video track_desc(
        track_id,
        rtc::Description::Direction::SendOnly);

    video_track_ = pc_->addTrack(track_desc);

    // Note: In a full implementation, we would set up a media handler
    // to feed video frames from IVideoSource to the track
    // For now, this is a placeholder for the track creation

  } catch (const std::exception& e) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (error_callback_) {
      error_callback_(std::string("Failed to start video track: ") + e.what());
    }
  }
}

void WebrtcTransportImpl::stopVideoTrack() {
  // Stop video source if it's still running
  if (video_source_) {
    video_source_->stop();
    video_source_ = nullptr;
  }

  // Close and reset video track
  if (video_track_) {
    video_track_->close();
    video_track_.reset();
  }
}

Result<void> WebrtcTransportImpl::sendDataChannelMessage(const std::string& message) {
  if (!dc_) {
    return Result<void>::make_error(
        ErrorType::kNetworkError, 1007, "Data channel not established");
  }

  if (message.empty()) {
    return Result<void>::make_error(
        ErrorType::kUnknownError, 2003, "Message is empty");
  }

  try {
    if (dc_->isOpen()) {
      dc_->send(message);
      return Result<void>::make_ok();
    } else {
      return Result<void>::make_error(
          ErrorType::kNetworkError, 1008, "Data channel is not open");
    }

  } catch (const std::exception& e) {
    return Result<void>::make_error(
        ErrorType::kNetworkError, 1009,
        std::string("Failed to send message: ") + e.what());
  }
}

ConnectionState WebrtcTransportImpl::getConnectionState() const noexcept {
  return state_.load(std::memory_order_acquire);
}

std::vector<IceCandidate> WebrtcTransportImpl::getLocalIceCandidates() const {
  std::lock_guard<std::mutex> lock(candidates_mutex_);
  return local_ice_candidates_;
}

void WebrtcTransportImpl::setStateChangeCallback(StateChangeCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  state_change_callback_ = std::move(callback);
}

void WebrtcTransportImpl::setIceCandidateCallback(IceCandidateCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  ice_candidate_callback_ = std::move(callback);
}

void WebrtcTransportImpl::setDataChannelCallback(DataChannelCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  data_channel_callback_ = std::move(callback);
}

void WebrtcTransportImpl::setErrorCallback(ErrorCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  error_callback_ = std::move(callback);
}

void WebrtcTransportImpl::setupPeerConnectionCallbacks() {
  if (!pc_) {
    return;
  }

  pc_->onLocalDescription([this](const rtc::Description& desc) {
    // SDP generated
  });

  pc_->onLocalCandidate([this](const rtc::Candidate& cand) {
    IceCandidate candidate;
    candidate.candidate = std::string(cand);
    candidate.sdp_mid = cand.mid();
    candidate.sdp_mline_index = 0;

    {
      std::lock_guard<std::mutex> lock(candidates_mutex_);
      local_ice_candidates_.push_back(candidate);
    }

    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (ice_candidate_callback_) {
      ice_candidate_callback_(candidate);
    }
  });

  pc_->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
    if (state == rtc::PeerConnection::GatheringState::Complete) {
      // ICE gathering complete
    }
  });

  pc_->onStateChange([this](rtc::PeerConnection::State state) {
    ConnectionState new_state = mapConnectionState(state);
    ConnectionState old_state = state_.exchange(new_state, std::memory_order_release);
    std::cout << "[WebrtcTransport] PeerConnection state changed: "
              << static_cast<int>(old_state) << " -> "
              << static_cast<int>(new_state) << " (libdatachannel: "
              << static_cast<int>(state) << ")" << std::endl;

    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (state_change_callback_) {
      state_change_callback_(new_state);
    }
  });

  pc_->onDataChannel([this](std::shared_ptr<rtc::DataChannel> remote_dc) {
    std::cout << "[WebrtcTransport] Received remote data channel: "
              << remote_dc->label() << std::endl;
    dc_ = remote_dc;
    setupDataChannelCallbacks();
  });
}

void WebrtcTransportImpl::setupDataChannelCallbacks() {
  if (!dc_) {
    return;
  }

  dc_->onOpen([this]() {
    std::cout << "[WebrtcTransport] Data channel opened" << std::endl;
    state_.store(ConnectionState::kConnected, std::memory_order_release);
  });

  dc_->onClosed([this]() {
    if (state_.load(std::memory_order_acquire) == ConnectionState::kConnected) {
      state_.store(ConnectionState::kDisconnected, std::memory_order_release);
    }
  });

  dc_->onMessage([this](std::variant<rtc::binary, rtc::string> message) {
    std::string msg_str;

    if (std::holds_alternative<rtc::string>(message)) {
      msg_str = std::get<rtc::string>(message);
    } else if (std::holds_alternative<rtc::binary>(message)) {
      const rtc::binary& binary = std::get<rtc::binary>(message);
      msg_str.resize(binary.size());
      std::transform(binary.begin(), binary.end(), msg_str.begin(),
                   [](std::byte b) { return static_cast<char>(b); });
    }

    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (data_channel_callback_) {
      data_channel_callback_(msg_str);
    }
  });

  dc_->onError([this](std::string error) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (error_callback_) {
      error_callback_("Data channel error: " + error);
    }
  });
}

ConnectionState WebrtcTransportImpl::mapConnectionState(
    rtc::PeerConnection::State state) const {
  switch (state) {
    case rtc::PeerConnection::State::New:
      return ConnectionState::kNew;
    case rtc::PeerConnection::State::Connecting:
      return ConnectionState::kChecking;
    case rtc::PeerConnection::State::Connected:
      return ConnectionState::kConnected;
    case rtc::PeerConnection::State::Failed:
      return ConnectionState::kFailed;
    case rtc::PeerConnection::State::Disconnected:
      return ConnectionState::kDisconnected;
    case rtc::PeerConnection::State::Closed:
      return ConnectionState::kClosed;
    default:
      return ConnectionState::kNew;
  }
}

// Factory functions
extern "C" SCREEN_STREAM_SDK_EXPORT IWebrtcTransport* CreateWebrtcTransport() {
  return new WebrtcTransportImpl();
}

extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyWebrtcTransport(
    IWebrtcTransport* transport) {
  if (transport != nullptr) {
    delete transport;
  }
}

} // namespace screensdk
