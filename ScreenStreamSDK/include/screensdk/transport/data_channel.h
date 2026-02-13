#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "screensdk/export.h"
#include "screensdk/utils/error.h"

namespace rtc {
class PeerConnection;
class DataChannel;
}

namespace screensdk {

/**
 * @brief DataChannel 连接状态
 */
enum class DataChannelState {
  kNew = 0,
  kConnecting = 1,
  kOpen = 2,
  kClosing = 3,
  kClosed = 4
};

/**
 * @brief SDP 类型
 */
enum class SdpType {
  kOffer = 0,
  kAnswer = 1
};

/**
 * @brief ICE gathering 状态
 */
enum class IceGatheringState {
  kNew = 0,
  kInProgress = 1,
  kComplete = 2
};

/**
 * @brief 数据接收回调
 */
using DataCallback = std::function<void(const std::vector<uint8_t>& data)>;

/**
 * @brief 状态变化回调
 */
using StateCallback = std::function<void(DataChannelState state)>;

/**
 * @brief ICE 候选回调（调用方需要传输给对方）
 */
using IceCandidateCallback = std::function<void(const std::string& candidate)>;

/**
 * @brief 本地描述回调（SDP offer/answer）
 */
using LocalDescriptionCallback = std::function<void(const std::string& sdp)>;

/**
 * @brief ICE gathering 状态变化回调
 */
using IceGatheringStateCallback = std::function<void(IceGatheringState state)>;

/**
 * @brief DataChannel 配置
 */
struct DataChannelConfig {
  std::string label = "default";
  std::string protocol = "";
  bool ordered = true;
  uint16_t maxRetransmits = 0;
};

/**
 * @brief WebRTC DataChannel 类
 *
 * 提供局域网内的 P2P 数据传输能力。
 * 信令交换（SDP + ICE candidates）由调用方完全控制。
 */
class DataChannel {
public:
  explicit DataChannel(const DataChannelConfig& config);
  ~DataChannel();

  DataChannel(const DataChannel&) = delete;
  DataChannel& operator=(const DataChannel&) = delete;
  DataChannel(DataChannel&&) = delete;
  DataChannel& operator=(DataChannel&&) = delete;

  /**
   * @brief 创建 SDP offer（被控端调用）
   */
  Result<std::string> createOffer();

  /**
   * @brief 创建 SDP answer（控制端调用）
   */
  Result<std::string> createAnswer();

  /**
   * @brief 设置远程 SDP 描述
   * @param sdp SDP 字符串
   * @param type SDP 类型（offer 或 answer）
   */
  Result<void> setRemoteDescription(const std::string& sdp, SdpType type = SdpType::kOffer);

  /**
   * @brief 添加远程 ICE 候选
   */
  Result<void> addIceCandidate(const std::string& candidate);

  /**
   * @brief 设置 ICE 候选回调
   */
  void onIceCandidate(IceCandidateCallback callback);

  /**
   * @brief 设置本地描述回调
   */
  void onLocalDescription(LocalDescriptionCallback callback);

  /**
   * @brief 设置 ICE gathering 状态变化回调
   */
  void onIceGatheringStateChange(IceGatheringStateCallback callback);

  /**
   * @brief 断开连接
   */
  void disconnect();

  /**
   * @brief 检查是否已连接
   */
  bool isConnected() const noexcept;

  /**
   * @brief 获取当前状态
   */
  DataChannelState getState() const noexcept;

  /**
   * @brief 获取 DataChannel 标签
   */
  std::string getLabel() const;

  /**
   * @brief 发送二进制数据
   */
  Result<void> send(const std::vector<uint8_t>& data);

  /**
   * @brief 发送文本数据
   */
  Result<void> send(const std::string& text);

  /**
   * @brief 设置数据接收回调
   */
  void setDataCallback(DataCallback callback);

  /**
   * @brief 设置状态变化回调
   */
  void setStateCallback(StateCallback callback);

private:
  void setupDataChannelCallbacks();

  DataChannelConfig config_;
  std::atomic<DataChannelState> state_{DataChannelState::kNew};
  IceCandidateCallback ice_candidate_callback_;
  LocalDescriptionCallback local_description_callback_;
  IceGatheringStateCallback ice_gathering_state_callback_;
  DataCallback data_callback_;
  StateCallback state_callback_;
  mutable std::mutex callback_mutex_;

  std::shared_ptr<rtc::PeerConnection> pc_;
  std::shared_ptr<rtc::DataChannel> dc_;
};

} // namespace screensdk
