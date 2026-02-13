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
 * @brief SDP 重新协商状态
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
 * @brief 本地描述回调
 */
using RenegotiationLocalDescriptionCallback = std::function<void(const std::string& sdp)>;

/**
 * @brief 状态变化回调
 */
using RenegotiationStateCallback = std::function<void(RenegotiationState state)>;

/**
 * @brief SDP 重新协商处理器
 *
 * U2: Implement SDP renegotiation handler
 *
 * 管理 WebRTC Session Description Protocol 重新协商，用于显示器切换。
 * 支持≤100ms 的无缝过渡。
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
   * @brief 开始显示器切换（触发重新协商）
   * @return Result 包含新 SDP offer 或错误
   */
  Result<std::string> initiateDisplaySwitch();

  /**
   * @brief 处理远程 SDP offer（控制端响应显示器切换请求）
   * @param sdp 远程 SDP offer
   * @return Result 包含 SDP answer 或错误
   */
  Result<std::string> handleRemoteOffer(const std::string& sdp);

  /**
   * @brief 处理远程 SDP answer（被控端完成重新协商）
   * @param sdp 远程 SDP answer
   * @return 成功或失败
   */
  Result<void> handleRemoteAnswer(const std::string& sdp);

  /**
   * @brief 获取当前状态
   */
  RenegotiationState getState() const noexcept;

  /**
   * @brief 检查是否正在进行重新协商
   */
  bool isRenegotiating() const noexcept;

  /**
   * @brief 设置本地描述回调
   */
  void onLocalDescription(RenegotiationLocalDescriptionCallback callback);

  /**
   * @brief 设置状态变化回调
   */
  void onStateChange(RenegotiationStateCallback callback);

  /**
   * @brief 重置协商状态
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
