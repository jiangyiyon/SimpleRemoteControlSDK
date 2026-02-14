#pragma once

#include <functional>
#include <string>
#include <vector>

#include "screensdk/export.h"
#include "screensdk/transport/video_source.h"
#include "screensdk/utils/error.h"

namespace screensdk {

/**
 * @brief SDP type enumeration
 */
enum class SdpType {
  kOffer = 0,
  kAnswer = 1
};

/**
 * @brief ICE candidate information
 */
struct IceCandidate {
  std::string candidate;
  std::string sdp_mid;
  int sdp_mline_index = 0;
};

/**
 * @brief WebRTC connection state
 */
enum class ConnectionState {
  kNew = 0,
  kChecking = 1,
  kConnected = 2,
  kCompleted = 3,
  kFailed = 4,
  kDisconnected = 5,
  kClosed = 6
};

/**
 * @brief WebRTC transport configuration
 */
struct TransportConfig {
  std::string stun_server = "stun:stun.l.google.com:19302";
  bool use_ipv6 = false;
  int max_bitrate_bps = 15000000;
  bool enable_ice_tcp = false;
};

/**
 * @brief Connection state change callback
 */
using StateChangeCallback = std::function<void(ConnectionState)>;

/**
 * @brief ICE candidate callback
 */
using IceCandidateCallback = std::function<void(const IceCandidate&)>;

/**
 * @brief Data channel message callback
 */
using DataChannelCallback = std::function<void(const std::string&)>;

/**
 * @brief Error callback
 */
using ErrorCallback = std::function<void(const std::string&)>;

/**
 * @brief WebRTC transport interface for peer-to-peer communication
 *
 * T015: Implement WebRTC manager wrapper with libwebrtc integration
 *
 * Provides WebRTC peer connection management for video streaming and
 * data channel communication between Windows host and mobile client.
 * Uses libdatachannel for WebRTC implementation.
 */
struct SCREEN_STREAM_SDK_EXPORT IWebrtcTransport {
  virtual ~IWebrtcTransport() = default;

  /**
   * @brief Initialize WebRTC transport with configuration
   * @param config Transport configuration (STUN server, bitrate, etc.)
   * @return true if initialization succeeded, false otherwise
   */
  virtual bool initialize(const TransportConfig& config) = 0;

  /**
   * @brief Shutdown WebRTC transport and release resources
   */
  virtual void shutdown() = 0;

  /**
   * @brief Create SDP offer for peer connection
   * @return SDP offer string, empty on error
   */
  virtual Result<std::string> createOffer() = 0;

  /**
   * @brief Set remote SDP description
   * @param sdp SDP offer or answer from peer
   * @param type Type of SDP (offer or answer)
   * @return true if successful, false on error
   */
  virtual Result<void> setRemoteDescription(const std::string& sdp,
                                             SdpType type = SdpType::kAnswer) = 0;

  /**
   * @brief Add remote ICE candidate
   * @param candidate ICE candidate from peer
   * @return true if successful, false on error
   */
  virtual Result<void> addIceCandidate(const IceCandidate& candidate) = 0;

  /**
   * @brief Start video track with specified source
   * @param source Video source to stream
   */
  virtual void startVideoTrack(IVideoSource* source) = 0;

  /**
   * @brief Stop video track
   */
  virtual void stopVideoTrack() = 0;

  /**
   * @brief Send data channel message to peer
   * @param message Message to send
   * @return true if successful, false on error
   */
  virtual Result<void> sendDataChannelMessage(const std::string& message) = 0;

  /**
   * @brief Get current connection state
   * @return Current connection state
   */
  virtual ConnectionState getConnectionState() const noexcept = 0;

  /**
   * @brief Get local ICE candidates
   * @return List of local ICE candidates
   */
  virtual std::vector<IceCandidate> getLocalIceCandidates() const = 0;

  /**
   * @brief Set state change callback
   * @param callback Callback function for state changes
   */
  virtual void setStateChangeCallback(StateChangeCallback callback) = 0;

  /**
   * @brief Set ICE candidate callback
   * @param callback Callback function for new ICE candidates
   */
  virtual void setIceCandidateCallback(IceCandidateCallback callback) = 0;

  /**
   * @brief Set data channel message callback
   * @param callback Callback function for received messages
   */
  virtual void setDataChannelCallback(DataChannelCallback callback) = 0;

  /**
   * @brief Set error callback
   * @param callback Callback function for errors
   */
  virtual void setErrorCallback(ErrorCallback callback) = 0;
};

/**
 * @brief Factory function to create WebRTC transport
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IWebrtcTransport* CreateWebrtcTransport();
extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyWebrtcTransport(IWebrtcTransport* transport);

} // namespace screensdk
