#include "screensdk/transport/i_webrtc_transport.h"
#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"

#include <rtc/rtc.h>
#include <rtc/peerconnection.hpp>
#include <rtc/global.hpp>
#include <rtc/datachannel.hpp>
#include <rtc/track.hpp>
#include <rtc/h264rtppacketizer.hpp>
#include <rtc/rtcpsrreporter.hpp>
#include <rtc/rtcpnackresponder.hpp>

#include <mutex>
#include <vector>
#include <cstring>
#include <iostream>

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
  void sendInitialNalus(const uint8_t* h264_data, size_t size) override;
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
  void sendEncodedFrame(const std::vector<uint8_t>& h264_data, uint64_t timestamp_us);

  TransportConfig config_;
  std::shared_ptr<rtc::PeerConnection> pc_;
  std::shared_ptr<rtc::DataChannel> dc_;
  std::shared_ptr<rtc::Track> video_track_;
  IVideoSource* video_source_ = nullptr;

  // Encoder and RTP configuration
  IVideoEncoder* encoder_ = nullptr;
  std::shared_ptr<rtc::RtpPacketizationConfig> rtp_config_;
  std::weak_ptr<rtc::RtcpSrReporter> sr_reporter_;

  // Initial NALU buffer for fast startup
  std::vector<uint8_t> initial_nalus_;
  bool initial_nalus_sent_ = false;
  bool track_opened_ = false;
  bool video_source_started_ = false;

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
  
  // Clean up encoder
  if (encoder_ != nullptr) {
    delete encoder_;
    encoder_ = nullptr;
  }
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

    // Force media transport initialization
    // This is required for sending tracks to work properly
    rtc_config.forceMediaTransport = config.force_media_transport;

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
  if (video_track_) {
    video_track_->close();
    video_track_.reset();
  }

  if (pc_) {
    pc_->close();
    pc_.reset();
  }

  if (dc_) {
    dc_->close();
    dc_.reset();
  }

  state_.store(ConnectionState::kClosed, std::memory_order_release);
  
  // Clear initial NALU buffer
  initial_nalus_.clear();
  initial_nalus_sent_ = false;
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

  std::cout << "[WebrtcTransport] ========== Adding remote ICE candidate ==========" << std::endl;
  std::cout << "[WebrtcTransport] Candidate: " << candidate.candidate.substr(0, 100) << "..." << std::endl;
  std::cout << "[WebrtcTransport] MID: " << candidate.sdp_mid << std::endl;
  std::cout << "[WebrtcTransport] Current connection state: "
            << static_cast<int>(state_.load(std::memory_order_acquire)) << std::endl;

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
    std::cout << "[WebrtcTransport] ================================================" << std::endl;
    return Result<void>::make_ok();

  } catch (const std::exception& e) {
    std::cerr << "[WebrtcTransport] Exception in addIceCandidate: "
              << e.what() << std::endl;
    std::cerr << "[WebrtcTransport] ================================================" << std::endl;
    return Result<void>::make_error(
        ErrorType::kNetworkError, 1006,
        std::string("Failed to add ICE candidate: ") + e.what());
  }
}

void WebrtcTransportImpl::startVideoTrack(IVideoSource* source) {
  if (!pc_ || !source) {
    return;
  }

  if (video_track_) {
    stopVideoTrack();
  }

  try {
    video_source_ = source;

    // Get video source parameters
    int width = 0, height = 0;
    int fps = source->getFps();
    source->getFrameSize(&width, &height);

    std::cout << "[WebrtcTransport] Starting video track: " 
              << width << "x" << height << " @ " << fps << "fps" << std::endl;

    // RTP/H.264 configuration
    const uint8_t kPayloadType = 102;
    const uint32_t kSsrc = 1;
    const std::string kCname = "video-stream";
    const std::string kMsid = "stream1";

    // Create video track with H.264 codec
    // Use "0" as mid to match typical client offer video media mid
    std::string track_id = "video_" + std::to_string(reinterpret_cast<uintptr_t>(source));
    rtc::Description::Video video_desc("0");  // Use "0" to match client's video media mid
    video_desc.addH264Codec(kPayloadType);
    video_desc.addSSRC(kSsrc, kCname, kMsid, kCname);

    video_track_ = pc_->addTrack(video_desc);

    if (!video_track_) {
      std::cerr << "[WebrtcTransport] Failed to add video track to peer connection" << std::endl;
      return;
    }

    std::cout << "[WebrtcTransport] Video track created successfully, track ID: "
              << video_track_->mid() << std::endl;

    // Create RTP configuration
    rtp_config_ = std::make_shared<rtc::RtpPacketizationConfig>(
        kSsrc, kCname, kPayloadType, rtc::H264RtpPacketizer::ClockRate);

    // Create H.264 RTP packetizer with Annex-B separator
    // Note: x264 encoder outputs Annex-B format (start codes: 00 00 00 01 or 00 00 01)
    // So we must use Annex-B separator, not Length separator
    auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(
        rtc::NalUnit::Separator::StartSequence, rtp_config_);
    
    // Add RTCP SR handler for synchronization
    auto sr_reporter = std::make_shared<rtc::RtcpSrReporter>(rtp_config_);
    sr_reporter_ = sr_reporter;
    packetizer->addToChain(sr_reporter);
    
    // Add RTCP NACK handler for packet retransmission
    auto nack_responder = std::make_shared<rtc::RtcpNackResponder>();
    packetizer->addToChain(nack_responder);
    
    // Set media handler
    video_track_->setMediaHandler(packetizer);

    // Create encoder using factory
    std::cout << "[WebrtcTransport] Creating encoder factory..." << std::endl;
    auto factory = CreateEncoderFactory();
    if (factory == nullptr) {
      std::cerr << "[WebrtcTransport] Failed to create encoder factory" << std::endl;
      return;
    }
    std::cout << "[WebrtcTransport] Encoder factory created successfully" << std::endl;

    encoder_ = factory->createEncoder();
    DestroyEncoderFactory(factory);

    if (encoder_ == nullptr) {
      std::cerr << "[WebrtcTransport] Failed to create encoder" << std::endl;
      return;
    }
    std::cout << "[WebrtcTransport] Encoder created successfully" << std::endl;

    // Initialize encoder with default configuration
    EncoderConfig encoder_config;
    encoder_config.width = width;
    encoder_config.height = height;
    encoder_config.fps = fps;
    encoder_config.bitrate = config_.max_bitrate_bps;
    encoder_config.gop_size = fps; // 1 second GOP
    encoder_config.b_frames = 0;   // Low latency
    encoder_config.preset = "veryfast";
    encoder_config.tune = "zerolatency";

    std::cout << "[WebrtcTransport] Initializing encoder with config: "
              << width << "x" << height << " @ " << fps << "fps, bitrate: "
              << config_.max_bitrate_bps << " bps" << std::endl;

    std::string config_json = encoder_config.toJson();
    if (!encoder_->initialize(width, height, fps, config_json)) {
      std::cerr << "[WebrtcTransport] Failed to initialize encoder" << std::endl;
      delete encoder_;
      encoder_ = nullptr;
      return;
    }
    std::cout << "[WebrtcTransport] Encoder initialized successfully" << std::endl;

    // Set up frame callback from video source
    std::cout << "[WebrtcTransport] Setting up frame callback for video source..." << std::endl;
    source->setFrameCallback([this](const VideoFrameForTrans& frame) {
      static int callback_count = 0;
      callback_count++;

      if (!video_track_) {
        static int log_count = 0;
        if (log_count < 5) {
          std::cout << "[WebrtcTransport] Frame callback #" << callback_count
                    << ": video_track_ is nullptr" << std::endl;
          log_count++;
        }
        return;
      }

      // Note: For sending tracks, isOpen() returns false until DTLS-SRTP is established
      // But we can still send frames - they will be buffered and sent when ready
      // So we DON'T check isOpen() for sending tracks

      if (!encoder_) {
        std::cerr << "[WebrtcTransport] Frame callback #" << callback_count
                  << ": encoder_ is nullptr" << std::endl;
        return;
      }

      // Encode BGRA frame to H.264
      std::vector<uint8_t> h264_buffer(frame.width * frame.height * 2);
      size_t encoded_size = h264_buffer.size();

      static int frame_count = 0;
      bool encode_success = encoder_->encode(frame, h264_buffer.data(), &encoded_size);

      if (encode_success && encoded_size > 0) {
        frame_count++;

        // Log first frame
        if (frame_count == 1) {
          std::cout << "[WebrtcTransport] ========== First frame encoded ==========" << std::endl;
          std::cout << "[WebrtcTransport] Frame size: " << encoded_size << " bytes" << std::endl;
          std::cout << "[WebrtcTransport] =========================================" << std::endl;
        }

        // Store initial keyframe if not sent yet
        if (!initial_nalus_sent_ && initial_nalus_.empty()) {
          // Save first keyframe for fast startup
          initial_nalus_.assign(h264_buffer.begin(), h264_buffer.begin() + encoded_size);
          std::cout << "[WebrtcTransport] Stored initial NALUs, size: " << initial_nalus_.size() << std::endl;
        }

        // Send encoded frame
        h264_buffer.resize(encoded_size);
        sendEncodedFrame(h264_buffer, frame.timestamp_ms * 1000); // Convert ms to us

        // Log every 30 frames
        if (frame_count % 30 == 0) {
          std::cout << "[WebrtcTransport] Sent " << frame_count << " frames, last frame size: "
                    << encoded_size << " bytes" << std::endl;
        }
      } else {
        static int error_count = 0;
        if (error_count < 5) {
          std::cerr << "[WebrtcTransport] Frame encoding failed, frame size: "
                    << frame.width << "x" << frame.height << ", encoded_size: "
                    << encoded_size << ", callback_count: " << callback_count << std::endl;
          error_count++;
        }
      }
    });

    std::cout << "[WebrtcTransport] Frame callback setup completed" << std::endl;

    // Set up onOpen callback BEFORE any other callbacks
    std::cout << "[WebrtcTransport] Setting up onOpen callback for video track..." << std::endl;
    video_track_->onOpen([this]() {
      std::cout << "[WebrtcTransport] ========== Video track OPENED (onOpen callback) ==========" << std::endl;
      std::cout << "[WebrtcTransport] Track is open, isOpen(): " << video_track_->isOpen() << std::endl;
      track_opened_ = true;

      // Send initial NALUs to reduce first frame delay
      if (!initial_nalus_sent_ && !initial_nalus_.empty()) {
        std::cout << "[WebrtcTransport] Sending initial NALUs, size: " << initial_nalus_.size() << " bytes" << std::endl;
        sendInitialNalus(initial_nalus_.data(), initial_nalus_.size());
        initial_nalus_sent_ = true;
        std::cout << "[WebrtcTransport] Initial NALUs sent successfully" << std::endl;
      } else {
        if (initial_nalus_sent_) {
          std::cout << "[WebrtcTransport] Initial NALUs already sent" << std::endl;
        } else {
          std::cout << "[WebrtcTransport] No initial NALUs available to send" << std::endl;
        }
      }

      // Start video source
      if (video_source_ && !video_source_started_) {
        std::cout << "[WebrtcTransport] Starting video source..." << std::endl;
        video_source_->start();
        video_source_started_ = true;
        std::cout << "[WebrtcTransport] Video source started" << std::endl;
      } else {
        if (video_source_started_) {
          std::cout << "[WebrtcTransport] Video source already started" << std::endl;
        } else {
          std::cerr << "[WebrtcTransport] Video source is nullptr, cannot start" << std::endl;
        }
      }

      std::cout << "[WebrtcTransport] ================================================" << std::endl;
    });

    std::cout << "[WebrtcTransport] onOpen callback setup completed" << std::endl;

    std::cout << "[WebrtcTransport] Setting up onClosed callback for video track..." << std::endl;
    video_track_->onClosed([this]() {
      std::cout << "[WebrtcTransport] ========== Video track CLOSED (onClosed callback) ==========" << std::endl;
      std::cout << "[WebrtcTransport] Track closed" << std::endl;
      if (video_source_) {
        std::cout << "[WebrtcTransport] Stopping video source..." << std::endl;
        video_source_->stop();
        std::cout << "[WebrtcTransport] Video source stopped" << std::endl;
      }
      std::cout << "[WebrtcTransport] ================================================" << std::endl;
    });

    std::cout << "[WebrtcTransport] Setting up onError callback for video track..." << std::endl;
    video_track_->onError([this](std::string error) {
      std::cerr << "[WebrtcTransport] ========== Video track ERROR (onError callback) ==========" << std::endl;
      std::cerr << "[WebrtcTransport] Track error: " << error << std::endl;
      std::cerr << "[WebrtcTransport] =============================================" << std::endl;
    });

    std::cout << "[WebrtcTransport] ========== Video track setup completed ==========" << std::endl;
    std::cout << "[WebrtcTransport] Track status check - isOpen: " << video_track_->isOpen()
              << ", isClosed: " << video_track_->isClosed() << std::endl;

    std::cout << "[WebrtcTransport] ==================================================" << std::endl;

    // Note: Video source should be started by the caller at the appropriate time
    // (e.g., when a client connects and we're ready to send)
    // We don't auto-start here to allow the caller to control when capture begins
    std::cout << "[WebrtcTransport] Video track setup completed, waiting for external start signal" << std::endl;
    std::cout << "[WebrtcTransport] ==================================================" << std::endl;

  } catch (const std::exception& e) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (error_callback_) {
      error_callback_(std::string("Failed to start video track: ") + e.what());
    }
  }
}

void WebrtcTransportImpl::stopVideoTrack() {
  std::cout << "[WebrtcTransport] ========== stopVideoTrack called ==========" << std::endl;

  // Stop video source if it's still running
  if (video_source_) {
    std::cout << "[WebrtcTransport] Stopping video source..." << std::endl;
    video_source_->stop();
    video_source_ = nullptr;
    video_source_started_ = false;
    std::cout << "[WebrtcTransport] Video source stopped" << std::endl;
  } else {
    std::cout << "[WebrtcTransport] Video source is nullptr" << std::endl;
  }

  // Close and reset video track
  if (video_track_) {
    std::cout << "[WebrtcTransport] Closing video track..." << std::endl;
    video_track_->close();
    video_track_.reset();
    track_opened_ = false;
    std::cout << "[WebrtcTransport] Video track closed and reset" << std::endl;
  } else {
    std::cout << "[WebrtcTransport] Video track is nullptr" << std::endl;
  }

  // Clear RTP config
  rtp_config_.reset();

  // Flush encoder
  if (encoder_ != nullptr) {
    std::cout << "[WebrtcTransport] Flushing encoder..." << std::endl;
    encoder_->flush();
    std::cout << "[WebrtcTransport] Encoder flushed" << std::endl;
  }

  // Clear initial NALU buffer
  initial_nalus_.clear();
  initial_nalus_sent_ = false;

  std::cout << "[WebrtcTransport] ==========================================" << std::endl;
}

void WebrtcTransportImpl::sendInitialNalus(const uint8_t* h264_data, size_t size) {
  if (!video_track_ || !video_track_->isOpen() || h264_data == nullptr || size == 0) {
    return;
  }

  try {
    std::cout << "[WebrtcTransport] Sending initial NALUs, size: " << size << std::endl;
    
    // Send initial NALUs twice (Firefox compatibility)
    std::vector<std::byte> data(size);
    std::memcpy(data.data(), h264_data, size);
    video_track_->send(data);
    video_track_->send(data);
    
  } catch (const std::exception& e) {
    std::cerr << "[WebrtcTransport] Failed to send initial NALUs: " << e.what() << std::endl;
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

void WebrtcTransportImpl::sendEncodedFrame(const std::vector<uint8_t>& h264_data, uint64_t timestamp_us) {
  static int send_count = 0;
  static int error_count = 0;
  static int warning_count = 0;

  if (!video_track_) {
    return;
  }

  // Check track status before sending
  bool is_open = video_track_->isOpen();
  bool is_closed = video_track_->isClosed();

  // Log track status periodically
  if (send_count == 0 || (send_count % 100 == 0)) {
    std::cout << "[WebrtcTransport] Track status - isOpen: " << is_open
              << ", isClosed: " << is_closed
              << ", track_opened_: " << track_opened_
              << ", send_count: " << send_count << std::endl;
  }

  // For sending tracks, we need to wait for DTLS-SRTP transport to be ready
  // The track->open() must be called by PeerConnection before we can send frames
  if (!is_open) {
    if (warning_count < 10) {
      std::cout << "[WebrtcTransport] Frame #" << send_count
                << ": Track is not open yet, skipping send (isOpen=" << is_open
                << ", track_opened_=" << track_opened_ << ")" << std::endl;
      warning_count++;
    }
    // Don't increment send_count since we didn't actually send
    return;
  }

  try {
    // Convert to binary for libdatachannel
    std::vector<std::byte> data(h264_data.size());
    std::memcpy(data.data(), h264_data.data(), h264_data.size());

    // Create frame info with timestamp in seconds
    std::chrono::duration<double> timestamp_seconds(timestamp_us / 1000000.0);
    rtc::FrameInfo info = rtc::FrameInfo(timestamp_seconds);

    // Send frame with timestamp
    video_track_->sendFrame(data, info);

    send_count++;

    // Log first few sends
    if (send_count <= 5) {
      std::cout << "[WebrtcTransport] sendEncodedFrame: sent frame #" << send_count
                << ", size: " << h264_data.size() << " bytes" << std::endl;
    }

  } catch (const std::exception& e) {
    static int error_count = 0;
    if (error_count < 5) {
      std::cerr << "[WebrtcTransport] Failed to send encoded frame: " << e.what() << std::endl;
      error_count++;
    }
  }
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

    std::cout << "[WebrtcTransport] ========== Local ICE candidate generated ==========" << std::endl;
    std::cout << "[WebrtcTransport] Candidate: " << candidate.candidate.substr(0, 100) << "..." << std::endl;
    std::cout << "[WebrtcTransport] MID: " << candidate.sdp_mid << std::endl;
    std::cout << "[WebrtcTransport] ====================================================" << std::endl;

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
    std::cout << "[WebrtcTransport] ========== ICE gathering state changed ==========" << std::endl;
    std::cout << "[WebrtcTransport] Gathering state: " << static_cast<int>(state);
    if (state == rtc::PeerConnection::GatheringState::New) {
      std::cout << " (New)";
    } else if (state == rtc::PeerConnection::GatheringState::InProgress) {
      std::cout << " (InProgress)";
    } else if (state == rtc::PeerConnection::GatheringState::Complete) {
      std::cout << " (Complete) - ICE gathering finished";
    }
    std::cout << std::endl;
    std::cout << "[WebrtcTransport] ====================================================" << std::endl;
  });

  pc_->onIceStateChange([this](rtc::PeerConnection::IceState state) {
    std::cout << "[WebrtcTransport] ========== ICE state changed ==========" << std::endl;
    std::cout << "[WebrtcTransport] ICE state: " << static_cast<int>(state);
    switch (state) {
      case rtc::PeerConnection::IceState::New:
        std::cout << " (New)";
        break;
      case rtc::PeerConnection::IceState::Checking:
        std::cout << " (Checking)";
        break;
      case rtc::PeerConnection::IceState::Connected:
        std::cout << " (Connected) - ICE connection established!";
        std::cout << "\n[WebrtcTransport] IMPORTANT: ICE connected means DTLS transport will be initialized";
        break;
      case rtc::PeerConnection::IceState::Completed:
        std::cout << " (Completed)";
        break;
      case rtc::PeerConnection::IceState::Failed:
        std::cout << " (Failed) - ICE connection failed!";
        break;
      case rtc::PeerConnection::IceState::Disconnected:
        std::cout << " (Disconnected)";
        break;
      case rtc::PeerConnection::IceState::Closed:
        std::cout << " (Closed)";
        break;
      default:
        std::cout << " (Unknown)";
    }
    std::cout << std::endl;
    std::cout << "[WebrtcTransport] ==========================================" << std::endl;

    // Check video track status after ICE state changes
    if (state == rtc::PeerConnection::IceState::Connected) {
      std::cout << "[WebrtcTransport] ICE connected, checking video track..." << std::endl;
      if (video_track_) {
        std::cout << "[WebrtcTransport] Video track isOpen: " << video_track_->isOpen()
                  << ", track_opened_: " << track_opened_ << std::endl;
      } else {
        std::cout << "[WebrtcTransport] Video track is nullptr" << std::endl;
      }
    }
  });

  pc_->onStateChange([this](rtc::PeerConnection::State state) {
    ConnectionState new_state = mapConnectionState(state);
    ConnectionState old_state = state_.exchange(new_state, std::memory_order_release);
    std::cout << "[WebrtcTransport] ========== PeerConnection state changed ==========" << std::endl;
    std::cout << "[WebrtcTransport] State: " << static_cast<int>(old_state)
              << " -> " << static_cast<int>(new_state) << " (libdatachannel: "
              << static_cast<int>(state) << ")" << std::endl;

    // Print human-readable state name
    std::cout << "[WebrtcTransport] Human-readable state: ";
    switch (state) {
      case rtc::PeerConnection::State::New:
        std::cout << "New";
        break;
      case rtc::PeerConnection::State::Connecting:
        std::cout << "Connecting";
        break;
      case rtc::PeerConnection::State::Connected:
        std::cout << "Connected";
        break;
      case rtc::PeerConnection::State::Disconnected:
        std::cout << "Disconnected";
        break;
      case rtc::PeerConnection::State::Failed:
        std::cout << "Failed";
        break;
      case rtc::PeerConnection::State::Closed:
        std::cout << "Closed";
        break;
      default:
        std::cout << "Unknown (" << static_cast<int>(state) << ")";
    }
    std::cout << std::endl;

    // Check video track status when state changes
    if (video_track_) {
      std::cout << "[WebrtcTransport] Video track status - isOpen: " << video_track_->isOpen()
                << ", isClosed: " << video_track_->isClosed()
                << ", track_opened_: " << track_opened_ << std::endl;
    } else {
      std::cout << "[WebrtcTransport] Video track is nullptr" << std::endl;
    }

    std::cout << "[WebrtcTransport] ================================================" << std::endl;

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
