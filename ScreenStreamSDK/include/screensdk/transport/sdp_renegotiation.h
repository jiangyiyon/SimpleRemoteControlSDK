#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "screensdk/export.h"
#include "screensdk/utils/error.h"

namespace rtc {
class PeerConnection;
}

namespace screensdk {

/**
 * @brief SDP renegotiation state
 */
enum class RenegotiationState {
  kIdle = 0,
  kCreatingOffer = 1,
  kOfferCreated = 2,
  kSettingRemoteAnswer = 3,
  kCreatingAnswer = 4,
  kAnswerCreated = 5,
  kSettingRemoteOffer = 6,
  kReady = 7
};

/**
 * @brief Local description callback
 */
using RenegotiationLocalDescriptionCallback = std::function<void(const std::string& sdp)>;

/**
 * @brief State change callback
 */
using RenegotiationStateCallback = std::function<void(RenegotiationState state)>;

/**
 * @brief SDP renegotiation handler
 *
 * U2: Implement SDP renegotiation handler
 *
 * Manages WebRTC Session Description Protocol renegotiation for display switching.
 * Supports seamless transition within 100ms.
 */
class SdpRenegotiation {
public:
  explicit SdpRenegotiation(std::shared_ptr<rtc::PeerConnection> peer_connection);
  ~SdpRenegotiation();

  SdpRenegotiation(const SdpRenegotiation&) = delete;
  SdpRenegotiation& operator=(const SdpRenegotiation&) = delete;
  SdpRenegotiation(SdpRenegotiation&&) = delete;
  SdpRenegotiation& operator=(SdpRenegotiation&&) = delete;

  /**
   * @brief Start display switch (triggers renegotiation)
   * @return Result containing new SDP offer or error
   */
  Result<std::string> initiateDisplaySwitch();

  /**
   * @brief Handle remote SDP offer (controller responds to display switch request)
   * @param sdp Remote SDP offer
   * @return Result containing SDP answer or error
   */
  Result<std::string> handleRemoteOffer(const std::string& sdp);

  /**
   * @brief Handle remote SDP answer (controller completes renegotiation)
   * @param sdp Remote SDP answer
   * @return Success or failure
   */
  Result<void> handleRemoteAnswer(const std::string& sdp);

  /**
   * @brief Get current state
   */
  RenegotiationState getState() const noexcept;

  /**
   * @brief Check if renegotiation is in progress
   */
  bool isRenegotiating() const noexcept;

  /**
   * @brief Set local description callback
   */
  void onLocalDescription(RenegotiationLocalDescriptionCallback callback);

  /**
   * @brief Set state change callback
   */
  void onStateChange(RenegotiationStateCallback callback);

  /**
   * @brief Reset negotiation state
   */
  void reset();

private:
  void updateState(RenegotiationState new_state);
  void triggerLocalDescriptionCallback(const std::string& sdp);

  std::shared_ptr<rtc::PeerConnection> peer_connection_;
  std::atomic<RenegotiationState> state_{RenegotiationState::kIdle};
  RenegotiationLocalDescriptionCallback local_description_callback_;
  RenegotiationStateCallback state_callback_;
  mutable std::mutex callback_mutex_;
};

} // namespace screensdk
