#include "screensdk/transport/sdp_renegotiation.h"
#include <rtc/rtc.h>
#include <rtc/peerconnection.hpp>

namespace screensdk {

SdpRenegotiation::SdpRenegotiation(std::shared_ptr<rtc::PeerConnection> peer_connection)
    : peer_connection_(std::move(peer_connection)) {
  if (!peer_connection_) {
    throw std::invalid_argument("peer_connection cannot be null");
  }
}

SdpRenegotiation::~SdpRenegotiation() {
  reset();
}

Result<std::string> SdpRenegotiation::initiateDisplaySwitch() {
  std::lock_guard<std::mutex> lock(callback_mutex_);

  if (isRenegotiating()) {
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 0, "Renegotiation already in progress");
  }

  updateState(RenegotiationState::kCreatingOffer);

  // Create new SDP offer for display switch (synchronous)
  try {
    rtc::Description offer = peer_connection_->createOffer();
    updateState(RenegotiationState::kOfferCreated);
    triggerLocalDescriptionCallback(offer.generateSdp());

    return Result<std::string>::make_ok(offer.generateSdp());
  } catch (const std::exception& e) {
    updateState(RenegotiationState::kIdle);
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 0, std::string("Failed to create offer: ") + e.what());
  }
}

Result<std::string> SdpRenegotiation::handleRemoteOffer(const std::string& sdp) {
  std::lock_guard<std::mutex> lock(callback_mutex_);

  if (sdp.empty()) {
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 0, "SDP cannot be empty");
  }

  auto current_state = state_.load(std::memory_order_acquire);
  if (current_state != RenegotiationState::kIdle &&
      current_state != RenegotiationState::kReady) {
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 0, "Invalid state for handling remote offer");
  }

  updateState(RenegotiationState::kSettingRemoteOffer);

  // Set remote offer
  try {
    rtc::Description offer(sdp, "offer");
    peer_connection_->setRemoteDescription(offer);
  } catch (const std::exception& e) {
    updateState(RenegotiationState::kIdle);
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 0, std::string("Failed to set remote offer: ") + e.what());
  }

  // Create answer (synchronous)
  updateState(RenegotiationState::kCreatingAnswer);
  try {
    rtc::Description answer = peer_connection_->createAnswer();
    updateState(RenegotiationState::kAnswerCreated);
    triggerLocalDescriptionCallback(answer.generateSdp());

    return Result<std::string>::make_ok(answer.generateSdp());
  } catch (const std::exception& e) {
    updateState(RenegotiationState::kIdle);
    return Result<std::string>::make_error(
        ErrorType::kNetworkError, 0, std::string("Failed to create answer: ") + e.what());
  }
}

Result<void> SdpRenegotiation::handleRemoteAnswer(const std::string& sdp) {
  std::lock_guard<std::mutex> lock(callback_mutex_);

  if (sdp.empty()) {
    return Result<void>::make_error(
        ErrorType::kNetworkError, 0, "SDP cannot be empty");
  }

  if (state_.load(std::memory_order_acquire) != RenegotiationState::kOfferCreated) {
    return Result<void>::make_error(
        ErrorType::kNetworkError, 0, "Invalid state for handling remote answer");
  }

  updateState(RenegotiationState::kSettingRemoteAnswer);

  // Set remote answer
  try {
    rtc::Description answer(sdp, "answer");
    peer_connection_->setRemoteDescription(answer);
    updateState(RenegotiationState::kReady);
    return Result<void>::make_ok();
  } catch (const std::exception& e) {
    updateState(RenegotiationState::kIdle);
    return Result<void>::make_error(
        ErrorType::kNetworkError, 0, std::string("Failed to set remote answer: ") + e.what());
  }
}

RenegotiationState SdpRenegotiation::getState() const noexcept {
  return state_.load(std::memory_order_acquire);
}

bool SdpRenegotiation::isRenegotiating() const noexcept {
  auto state = state_.load(std::memory_order_acquire);
  return state != RenegotiationState::kIdle && state != RenegotiationState::kReady;
}

void SdpRenegotiation::onLocalDescription(RenegotiationLocalDescriptionCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  local_description_callback_ = std::move(callback);
}

void SdpRenegotiation::onStateChange(RenegotiationStateCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  state_callback_ = std::move(callback);
}

void SdpRenegotiation::reset() {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  updateState(RenegotiationState::kIdle);
}

void SdpRenegotiation::updateState(RenegotiationState new_state) {
  state_.store(new_state, std::memory_order_release);

  std::lock_guard<std::mutex> lock(callback_mutex_);
  if (state_callback_) {
    state_callback_(new_state);
  }
}

void SdpRenegotiation::triggerLocalDescriptionCallback(const std::string& sdp) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  if (local_description_callback_) {
    local_description_callback_(sdp);
  }
}

} // namespace screensdk
