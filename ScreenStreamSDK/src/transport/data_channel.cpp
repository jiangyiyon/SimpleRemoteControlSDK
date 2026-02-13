#include "screensdk/transport/data_channel.h"
#include <rtc/rtc.h>
#include <rtc/peerconnection.hpp>
#include <rtc/global.hpp>

namespace screensdk {

// Initialize libdatachannel once at program startup
namespace {
struct LibdatachannelInitializer {
  LibdatachannelInitializer() {
    // Enable logging for debugging
    rtc::InitLogger(rtc::LogLevel::Warning);
    rtc::Preload();
  }
};

static LibdatachannelInitializer g_initializer;
} // anonymous namespace

DataChannel::DataChannel(const DataChannelConfig& config)
    : config_(config), state_(DataChannelState::kNew) {
  // Create libdatachannel configuration (LAN only, no STUN/TURN)
  rtc::Configuration rtc_config;

  // Create PeerConnection
  pc_ = std::make_shared<rtc::PeerConnection>(rtc_config);

  // Set up PeerConnection callbacks
  pc_->onLocalDescription([this](const rtc::Description& desc) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (local_description_callback_) {
      local_description_callback_(desc.generateSdp());
    }
  });

  pc_->onLocalCandidate([this](const rtc::Candidate& cand) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (ice_candidate_callback_) {
      ice_candidate_callback_(std::string(cand));
    }
  });

  pc_->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (ice_gathering_state_callback_) {
      IceGatheringState mapped_state;
      switch (state) {
        case rtc::PeerConnection::GatheringState::New:
          mapped_state = IceGatheringState::kNew;
          break;
        case rtc::PeerConnection::GatheringState::InProgress:
          mapped_state = IceGatheringState::kInProgress;
          break;
        case rtc::PeerConnection::GatheringState::Complete:
          mapped_state = IceGatheringState::kComplete;
          break;
        default:
          return;
      }
      ice_gathering_state_callback_(mapped_state);
    }
  });

  pc_->onDataChannel([this](std::shared_ptr<rtc::DataChannel> remote_dc) {
    // Control side: receive DataChannel from controller
    dc_ = remote_dc;
    setupDataChannelCallbacks();
  });

  pc_->onStateChange([this](rtc::PeerConnection::State state) {
    if (state == rtc::PeerConnection::State::Connected) {
      // Will set to kOpen when DataChannel opens
    } else if (state == rtc::PeerConnection::State::Failed ||
               state == rtc::PeerConnection::State::Closed ||
               state == rtc::PeerConnection::State::Disconnected) {
      state_.store(DataChannelState::kClosed, std::memory_order_release);
      std::lock_guard<std::mutex> lock(callback_mutex_);
      if (state_callback_) {
        state_callback_(DataChannelState::kClosed);
      }
    }
  });
}

DataChannel::~DataChannel() {
  if (pc_) {
    pc_->close();
  }
}

Result<std::string> DataChannel::createOffer() {
  try {
    if (!pc_) {
      return Result<std::string>::make_error(ErrorType::kNetworkError, 1001,
                                               "PeerConnection not initialized");
    }

    if (dc_) {
      return Result<std::string>::make_error(ErrorType::kNetworkError, 1001,
                                               "DataChannel already created");
    }

    // Create DataChannel before creating offer
    rtc::DataChannelInit init;
    init.protocol = config_.protocol;
    if (config_.maxRetransmits > 0) {
      init.reliability.unordered = !config_.ordered;
      init.reliability.maxRetransmits = config_.maxRetransmits;
    }

    dc_ = pc_->createDataChannel(config_.label, init);
    if (!dc_) {
      return Result<std::string>::make_error(ErrorType::kNetworkError, 1001,
                                               "Failed to create DataChannel");
    }

    // Set up DataChannel callbacks
    setupDataChannelCallbacks();

    // Create offer
    rtc::Description offer = pc_->createOffer();

    std::string offer_str = offer.generateSdp();
    return Result<std::string>::make_ok(offer_str);
  } catch (const std::exception& e) {
    std::string msg = std::string("Failed to create offer: ") + e.what() +
                      " (typeid: " + typeid(e).name() + ")";
    return Result<std::string>::make_error(ErrorType::kNetworkError, 1001, msg);
  } catch (...) {
    return Result<std::string>::make_error(ErrorType::kNetworkError, 1001,
                                             "Failed to create offer: unknown exception");
  }
}

Result<std::string> DataChannel::createAnswer() {
  try {
    if (!pc_) {
      return Result<std::string>::make_error(ErrorType::kNetworkError, 1002,
                                               "PeerConnection not initialized");
    }

    // Control side: DataChannel will be received via onDataChannel callback
    // after setRemoteDescription is called with the offer

    // Create answer
    rtc::Description answer = pc_->createAnswer();
    return Result<std::string>::make_ok(std::string(answer));
  } catch (const std::exception& e) {
    return Result<std::string>::make_error(ErrorType::kNetworkError, 1002,
                                             std::string("Failed to create answer: ") + e.what());
  }
}

Result<void> DataChannel::setRemoteDescription(const std::string& sdp, SdpType type) {
  try {
    if (!pc_) {
      return Result<void>::make_error(ErrorType::kNetworkError, 1003,
                                       "PeerConnection not initialized");
    }

    std::string type_string = (type == SdpType::kOffer) ? "offer" : "answer";
    auto desc = rtc::Description(sdp, type_string);
    pc_->setRemoteDescription(desc);

    return Result<void>::make_ok();
  } catch (const std::exception& e) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1003,
                                     std::string("Failed to set remote description: ") + e.what());
  }
}

Result<void> DataChannel::addIceCandidate(const std::string& candidate) {
  try {
    if (!pc_) {
      return Result<void>::make_error(ErrorType::kNetworkError, 1004,
                                       "PeerConnection not initialized");
    }
    
    auto cand = rtc::Candidate(candidate, "0");
    pc_->addRemoteCandidate(cand);
    
    return Result<void>::make_ok();
  } catch (const std::exception& e) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1004,
                                     std::string("Failed to add ICE candidate: ") + e.what());
  }
}

void DataChannel::onIceCandidate(IceCandidateCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  ice_candidate_callback_ = std::move(callback);
}

void DataChannel::onLocalDescription(LocalDescriptionCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  local_description_callback_ = std::move(callback);
}

void DataChannel::onIceGatheringStateChange(IceGatheringStateCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  ice_gathering_state_callback_ = std::move(callback);
}

void DataChannel::disconnect() {
  if (dc_) {
    dc_->close();
  }
  
  if (pc_) {
    pc_->close();
  }
  
  auto old_state = state_.exchange(DataChannelState::kClosed, std::memory_order_acq_rel);
  
  std::lock_guard<std::mutex> lock(callback_mutex_);
  if (state_callback_ && old_state != DataChannelState::kClosed) {
    state_callback_(DataChannelState::kClosed);
  }
}

bool DataChannel::isConnected() const noexcept {
  return state_.load(std::memory_order_acquire) == DataChannelState::kOpen;
}

DataChannelState DataChannel::getState() const noexcept {
  return state_.load(std::memory_order_acquire);
}

std::string DataChannel::getLabel() const { return config_.label; }

Result<void> DataChannel::send(const std::vector<uint8_t>& data) {
  if (!dc_ || !dc_->isOpen()) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1005,
                                     "DataChannel not connected");
  }
  
  if (data.empty()) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1005,
                                     "Cannot send empty data");
  }
  
  rtc::binary binary_data;
  binary_data.reserve(data.size());
  for (auto b : data) {
    binary_data.push_back(static_cast<std::byte>(b));
  }
  
  if (!dc_->send(std::move(binary_data))) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1002,
                                     "Send failed (buffered)");
  }
  
  return Result<void>::make_ok();
}

Result<void> DataChannel::send(const std::string& text) {
  if (!dc_ || !dc_->isOpen()) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1006,
                                     "DataChannel not connected");
  }
  
  if (text.empty()) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1006,
                                     "Cannot send empty text");
  }
  
  if (!dc_->send(text)) {
    return Result<void>::make_error(ErrorType::kNetworkError, 1002,
                                     "Send failed (buffered)");
  }
  
  return Result<void>::make_ok();
}

void DataChannel::setDataCallback(DataCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  data_callback_ = std::move(callback);
}

void DataChannel::setStateCallback(StateCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  state_callback_ = std::move(callback);
}

void DataChannel::setupDataChannelCallbacks() {
  if (!dc_) {
    return;
  }
  
  dc_->onOpen([this]() {
    state_.store(DataChannelState::kOpen, std::memory_order_release);
    
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (state_callback_) {
      state_callback_(DataChannelState::kOpen);
    }
  });
  
  dc_->onClosed([this]() {
    state_.store(DataChannelState::kClosed, std::memory_order_release);
    
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (state_callback_) {
      state_callback_(DataChannelState::kClosed);
    }
  });
  
  dc_->onMessage([this](rtc::message_variant msg) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (!data_callback_) {
      return;
    }
    
    if (std::holds_alternative<rtc::binary>(msg)) {
      auto& binary = std::get<rtc::binary>(msg);
      std::vector<uint8_t> data;
      data.reserve(binary.size());
      for (auto b : binary) {
        data.push_back(static_cast<uint8_t>(b));
      }
      data_callback_(data);
    } else if (std::holds_alternative<std::string>(msg)) {
      auto& text = std::get<std::string>(msg);
      std::vector<uint8_t> data(text.begin(), text.end());
      data_callback_(data);
    }
  });
}

} // namespace screensdk
