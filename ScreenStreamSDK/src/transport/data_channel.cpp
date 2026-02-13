#include "screensdk/transport/data_channel.h"

namespace screensdk {

DataChannel::DataChannel(const DataChannelConfig& config)
    : config_(config), state_(DataChannelState::kNew) {}

DataChannel::~DataChannel() = default;

Result<std::string> DataChannel::createOffer() {
  return Result<std::string>::make_error(ErrorType::kNetworkError, 1001,
                                         "Not implemented");
}

Result<std::string> DataChannel::createAnswer() {
  return Result<std::string>::make_error(ErrorType::kNetworkError, 1002,
                                         "Not implemented");
}

Result<void> DataChannel::setRemoteDescription(const std::string& sdp) {
  (void)sdp;
  return Result<void>::make_error(ErrorType::kNetworkError, 1003,
                                   "Not implemented");
}

Result<void> DataChannel::addIceCandidate(const std::string& candidate) {
  (void)candidate;
  return Result<void>::make_error(ErrorType::kNetworkError, 1004,
                                   "Not implemented");
}

void DataChannel::onIceCandidate(IceCandidateCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  ice_candidate_callback_ = std::move(callback);
}

void DataChannel::onLocalDescription(LocalDescriptionCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  local_description_callback_ = std::move(callback);
}

void DataChannel::disconnect() {
  state_.store(DataChannelState::kClosed, std::memory_order_release);
}

bool DataChannel::isConnected() const noexcept {
  return state_.load(std::memory_order_acquire) == DataChannelState::kOpen;
}

DataChannelState DataChannel::getState() const noexcept {
  return state_.load(std::memory_order_acquire);
}

std::string DataChannel::getLabel() const { return config_.label; }

Result<void> DataChannel::send(const std::vector<uint8_t>& data) {
  (void)data;
  return Result<void>::make_error(ErrorType::kNetworkError, 1005,
                                   "Not implemented");
}

Result<void> DataChannel::send(const std::string& text) {
  (void)text;
  return Result<void>::make_error(ErrorType::kNetworkError, 1006,
                                   "Not implemented");
}

void DataChannel::setDataCallback(DataCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  data_callback_ = std::move(callback);
}

void DataChannel::setStateCallback(StateCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  state_callback_ = std::move(callback);
}

} // namespace screensdk
