#include "e2e_test_helper.h"

#include <iostream>

namespace screensdk {

// ============================================================================
// MockController Implementation
// ============================================================================

MockController::MockController() = default;

MockController::~MockController() {
  cleanup();
}

bool MockController::initialize() {
  DataChannelConfig config;
  config.label = "controller";
  config.ordered = true;

  data_channel_ = std::make_unique<DataChannel>(config);

  // Set up data callback to receive video frames
  data_channel_->setDataCallback([this](const std::vector<uint8_t>& data) {
    onDataReceived(data);
  });

  // Set up state callback
  data_channel_->setStateCallback([this](DataChannelState state) {
    if (state == DataChannelState::kOpen) {
      std::lock_guard<std::mutex> lock(frame_mutex_);
      connected_ = true;
      connected_cv_.notify_one();
    }
  });

  return true;
}

void MockController::cleanup() {
  if (data_channel_) {
    data_channel_->disconnect();
    data_channel_.reset();
  }
}

void MockController::onDataReceived(const std::vector<uint8_t>& data) {
  if (data.size() < sizeof(VideoFrameHeader)) {
    return;
  }

  // Parse frame header
  const VideoFrameHeader* header = reinterpret_cast<const VideoFrameHeader*>(data.data());

  // Validate magic number
  if (header->magic != 0x56465231) {
    return;
  }

  // Validate data size
  if (data.size() != sizeof(VideoFrameHeader) + header->data_size) {
    return;
  }

  // Extract frame data
  TimestampedFrame frame;
  frame.capture_time_ms = header->timestamp_ms;
  frame.frame_number = header->frame_number;
  frame.data.assign(data.begin() + sizeof(VideoFrameHeader), data.end());

  std::lock_guard<std::mutex> lock(frame_mutex_);
  received_frames_.push_back(frame);
  frame_count_++;
  frame_cv_.notify_one();
}

void MockController::waitForConnection(int timeout_ms) {
  std::unique_lock<std::mutex> lock(frame_mutex_);
  connected_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return connected_; });
}

void MockController::waitForFrame(int timeout_ms) {
  std::unique_lock<std::mutex> lock(frame_mutex_);
  frame_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                    [this] { return frame_count_ > 0; });
}

int MockController::getReceivedFrameCount() const {
  std::lock_guard<std::mutex> lock(frame_mutex_);
  return frame_count_;
}

uint64_t MockController::getLastFrameLatency() {
  std::lock_guard<std::mutex> lock(frame_mutex_);

  if (received_frames_.empty()) {
    return 0;
  }

  uint64_t current_time = MetricsCollector::getCurrentTimeMs();
  const auto& frame = received_frames_.back();
  return current_time - frame.capture_time_ms;
}

TimestampedFrame MockController::getLastFrame() {
  std::lock_guard<std::mutex> lock(frame_mutex_);

  if (received_frames_.empty()) {
    return TimestampedFrame{};
  }
  return received_frames_.back();
}

// ============================================================================
// MockRemoteHost Implementation
// ============================================================================

MockRemoteHost::MockRemoteHost()
  : encode_buffer_(10 * 1024 * 1024) {
}

MockRemoteHost::~MockRemoteHost() {
  cleanup();
}

bool MockRemoteHost::initialize(int display_index) {
  // Initialize display detector
  display_detector_ = std::make_unique<DisplayDetector>();
  auto displays = display_detector_->getDisplays();
  if (displays.empty()) {
    std::cerr << "No displays available" << std::endl;
    return false;
  }

  // Initialize capture
  capture_ = std::make_unique<DxgiCapture>();
  if (!capture_->initialize(display_index)) {
    std::cerr << "Failed to initialize DXGI capture" << std::endl;
    return false;
  }

  // Initialize encoder factory
  encoder_factory_.reset(CreateEncoderFactory());
  if (!encoder_factory_) {
    std::cerr << "Failed to create encoder factory" << std::endl;
    return false;
  }

  encoder_ = encoder_factory_->createEncoder(EncoderType::kSoftwareX264);
  if (!encoder_) {
    std::cerr << "Failed to create encoder" << std::endl;
    return false;
  }

  int width, height;
  capture_->getFrameSize(&width, &height);

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  if (!encoder_->initialize(width, height, capture_->getFps(), config_json)) {
    std::cerr << "Failed to initialize encoder" << std::endl;
    return false;
  }

  // Initialize DataChannel
  DataChannelConfig dc_config;
  dc_config.label = "remote-host";
  dc_config.ordered = true;

  data_channel_ = std::make_unique<DataChannel>(dc_config);

  // Set up state callback
  data_channel_->setStateCallback([this](DataChannelState state) {
    if (state == DataChannelState::kOpen) {
      std::lock_guard<std::mutex> lock(send_mutex_);
      connected_ = true;
      connected_cv_.notify_one();
    }
  });

  return true;
}

void MockRemoteHost::startStreaming() {
  if (!capture_ || !encoder_ || !data_channel_) {
    return;
  }

  streaming_ = true;
  capture_->setFrameCallback([this](const VideoFrame& frame) {
    onFrameCaptured(frame);
  });

  capture_->start();
}

void MockRemoteHost::stopStreaming() {
  streaming_ = false;
  if (capture_) {
    capture_->stop();
  }
}

void MockRemoteHost::cleanup() {
  stopStreaming();

  if (data_channel_) {
    data_channel_->disconnect();
    data_channel_.reset();
  }

  if (encoder_) {
    delete encoder_;
    encoder_ = nullptr;
  }

  if (encoder_factory_) {
    DestroyEncoderFactory(encoder_factory_.release());
  }

  capture_.reset();
  display_detector_.reset();
}

void MockRemoteHost::sendVideoFrame(const VideoFrame& frame) {
  if (!data_channel_ || !data_channel_->isConnected()) {
    return;
  }

  // Encode frame
  size_t output_size = encode_buffer_.size();
  if (!encoder_->encode(frame, encode_buffer_.data(), &output_size)) {
    return;
  }

  // Prepare frame header
  VideoFrameHeader header;
  header.magic = 0x56465231;
  header.frame_number = frame_number_;
  header.timestamp_ms = MetricsCollector::getCurrentTimeMs();
  header.width = frame.width;
  header.height = frame.height;
  header.data_size = static_cast<uint32_t>(output_size);

  // Combine header and data
  std::vector<uint8_t> packet(sizeof(VideoFrameHeader) + output_size);
  std::memcpy(packet.data(), &header, sizeof(VideoFrameHeader));
  std::memcpy(packet.data() + sizeof(VideoFrameHeader), encode_buffer_.data(), output_size);

  // Send packet
  data_channel_->send(packet);

  std::lock_guard<std::mutex> lock(send_mutex_);
  sent_frame_count_++;
  frame_number_++;
}

void MockRemoteHost::onFrameCaptured(const VideoFrame& frame) {
  if (streaming_ && connected_) {
    sendVideoFrame(frame);
  }
}

void MockRemoteHost::waitForConnection(int timeout_ms) {
  std::unique_lock<std::mutex> lock(send_mutex_);
  connected_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return connected_; });
}

int MockRemoteHost::getSentFrameCount() const {
  std::lock_guard<std::mutex> lock(send_mutex_);
  return sent_frame_count_;
}

// ============================================================================
// ConnectionHelper Implementation
// ============================================================================

bool ConnectionHelper::establishConnection(MockRemoteHost& remote,
                                           MockController& controller,
                                           int timeout_ms) {
  DataChannel* remote_dc = remote.getDataChannel();
  DataChannel* controller_dc = controller.getDataChannel();

  if (!remote_dc || !controller_dc) {
    std::cerr << "DataChannel not initialized" << std::endl;
    return false;
  }

  bool connected = exchangeSdpAndIce(*remote_dc, *controller_dc, timeout_ms);

  if (connected) {
    // Wait for connection
    remote.waitForConnection(5000);
    controller.waitForConnection(5000);

    connected = remote_dc->isConnected() && controller_dc->isConnected();
  }

  return connected;
}

bool ConnectionHelper::exchangeSdpAndIce(DataChannel& remote_dc,
                                         DataChannel& controller_dc,
                                         int timeout_ms) {
  std::string remote_offer;
  std::string controller_answer;

  std::vector<std::string> remote_ice_candidates;
  std::vector<std::string> controller_ice_candidates;

  // Exchange ICE candidates (set callbacks BEFORE creating offer/answer)
  bool remote_ice_done = false;
  bool controller_ice_done = false;
  bool remote_gathering_complete = false;
  bool controller_gathering_complete = false;

  remote_dc.onIceCandidate([&](const std::string& candidate) {
    if (!candidate.empty()) {
      remote_ice_candidates.push_back(candidate);
    } else {
      remote_ice_done = true;
    }
  });

  controller_dc.onIceCandidate([&](const std::string& candidate) {
    if (!candidate.empty()) {
      controller_ice_candidates.push_back(candidate);
    } else {
      controller_ice_done = true;
    }
  });

  // Use ICE gathering state callback to detect completion
  remote_dc.onIceGatheringStateChange([&](IceGatheringState state) {
    if (state == IceGatheringState::kComplete) {
      remote_gathering_complete = true;
    }
  });

  controller_dc.onIceGatheringStateChange([&](IceGatheringState state) {
    if (state == IceGatheringState::kComplete) {
      controller_gathering_complete = true;
    }
  });

  // Remote host creates offer
  auto offer_result = remote_dc.createOffer();
  if (!offer_result) {
    std::cerr << "Remote failed to create offer: " << offer_result.error().message << std::endl;
    return false;
  }
  remote_offer = offer_result.value();

  // Controller sets remote offer FIRST, then creates answer
  if (!controller_dc.setRemoteDescription(remote_offer, SdpType::kOffer)) {
    std::cerr << "Controller failed to set remote offer" << std::endl;
    return false;
  }

  // Controller creates answer
  auto answer_result = controller_dc.createAnswer();
  if (!answer_result) {
    std::cerr << "Controller failed to create answer: " << answer_result.error().message << std::endl;
    return false;
  }
  controller_answer = answer_result.value();

  // Remote sets remote answer
  if (!remote_dc.setRemoteDescription(controller_answer, SdpType::kAnswer)) {
    std::cerr << "Remote failed to set remote answer" << std::endl;
    return false;
  }

  // Wait for ICE gathering to complete (using state callback)
  auto start_time = std::chrono::steady_clock::now();
  while ((!remote_gathering_complete || !controller_gathering_complete)) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time).count();

    if (elapsed > timeout_ms) {
      std::cerr << "ICE gathering timeout after " << elapsed << "ms (remote=" << remote_gathering_complete
                << ", controller=" << controller_gathering_complete << ")" << std::endl;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  // Exchange ICE candidates
  for (const auto& candidate : remote_ice_candidates) {
    controller_dc.addIceCandidate(candidate);
  }

  for (const auto& candidate : controller_ice_candidates) {
    remote_dc.addIceCandidate(candidate);
  }

  return true;
}

} // namespace screensdk
