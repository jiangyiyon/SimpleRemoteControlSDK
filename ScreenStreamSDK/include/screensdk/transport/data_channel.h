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
 * @brief DataChannel connection state
 */
enum class DataChannelState {
  kNew = 0,
  kConnecting = 1,
  kOpen = 2,
  kClosing = 3,
  kClosed = 4
};

/**
 * @brief SDP type
 */
enum class SdpType {
  kOffer = 0,
  kAnswer = 1
};

/**
 * @brief ICE gathering state
 */
enum class IceGatheringState {
  kNew = 0,
  kInProgress = 1,
  kComplete = 2
};

/**
 * @brief Data receive callback
 */
using DataCallback = std::function<void(const std::vector<uint8_t>& data)>;

/**
 * @brief State change callback
 */
using StateCallback = std::function<void(DataChannelState state)>;

/**
 * @brief ICE candidate callback (caller needs to transmit to peer)
 */
using IceCandidateCallback = std::function<void(const std::string& candidate)>;

/**
 * @brief Local description callback (SDP offer/answer)
 */
using LocalDescriptionCallback = std::function<void(const std::string& sdp)>;

/**
 * @brief ICE gathering state change callback
 */
using IceGatheringStateCallback = std::function<void(IceGatheringState state)>;

/**
 * @brief DataChannel configuration
 */
struct DataChannelConfig {
  std::string label = "default";
  std::string protocol = "";
  bool ordered = true;
  uint16_t maxRetransmits = 0;
};

/**
 * @brief WebRTC DataChannel class
 *
 * Provides P2P data transmission capability within local network.
 * Signaling exchange (SDP + ICE candidates) is fully controlled by the caller.
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
   * @brief Create SDP offer (called by controlled end)
   */
  Result<std::string> createOffer();

  /**
   * @brief Create SDP answer (called by controller)
   */
  Result<std::string> createAnswer();

  /**
   * @brief Set remote SDP description
   * @param sdp SDP string
   * @param type SDP type (offer or answer)
   */
  Result<void> setRemoteDescription(const std::string& sdp, SdpType type = SdpType::kOffer);

  /**
   * @brief Add remote ICE candidate
   */
  Result<void> addIceCandidate(const std::string& candidate);

  /**
   * @brief Set ICE candidate callback
   */
  void onIceCandidate(IceCandidateCallback callback);

  /**
   * @brief Set local description callback
   */
  void onLocalDescription(LocalDescriptionCallback callback);

  /**
   * @brief Set ICE gathering state change callback
   */
  void onIceGatheringStateChange(IceGatheringStateCallback callback);

  /**
   * @brief Disconnect
   */
  void disconnect();

  /**
   * @brief Check if connected
   */
  bool isConnected() const noexcept;

  /**
   * @brief Get current state
   */
  DataChannelState getState() const noexcept;

  /**
   * @brief Get DataChannel label
   */
  std::string getLabel() const;

  /**
   * @brief Send binary data
   */
  Result<void> send(const std::vector<uint8_t>& data);

  /**
   * @brief Send text data
   */
  Result<void> send(const std::string& text);

  /**
   * @brief Set data receive callback
   */
  void setDataCallback(DataCallback callback);

  /**
   * @brief Set state change callback
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
