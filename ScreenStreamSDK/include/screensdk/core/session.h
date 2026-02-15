#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "screensdk/utils/types.h"
#include "screensdk/utils/error.h"

namespace screensdk {

// Forward declaration
namespace server {
class RemoteDesktopServer;
}

/**
 * @brief Remote desktop server configuration
 */
struct DesktopServerConfig {
    int http_port{8080};
    int signaling_port{8081};
    std::string web_root{"web"};
    int display_id{0};
    int fps{30};
    int max_bitrate_bps{15000000};
    std::string stun_server{"stun:stun.l.google.com:19302"};
};

/**
 * @brief Session entity for remote desktop connection
 *
 * T033: Implement Session class with state machine and latency tracking
 *
 * Represents a remote desktop connection between Windows host and mobile client.
 * Manages connection state, display selection, latency tracking, and lifecycle.
 */
class Session {
public:
  /**
   * @brief Default constructor
   *
   * Initializes session with DISCONNECTED state and generates unique session ID.
   */
  Session();

  /**
   * @brief Destructor
   */
  ~Session();

  // Disable copy and move operations
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;
  Session(Session&&) = delete;
  Session& operator=(Session&&) = delete;

  /**
   * @brief Get session ID (UUID v4)
   * @return Session ID string
   */
  std::string getSessionId() const;

  /**
   * @brief Get current session state
   * @return Current state
   */
  SessionState getState() const;

  /**
   * @brief Get client IP address
   * @return Client IP string, empty if not connected
   */
  std::string getClientIp() const;

  /**
   * @brief Get connection timestamp
   * @return Connection time point
   */
  std::chrono::system_clock::time_point getConnectedAt() const;

  /**
   * @brief Get last activity timestamp
   * @return Last activity time point
   */
  std::chrono::system_clock::time_point getLastActivity() const;

  /**
   * @brief Get selected display source ID
   * @return Display ID (0-3), or -1 if not selected
   */
  int getDisplaySourceId() const;

  /**
   * @brief Get current latency in milliseconds
   * @return Latency in milliseconds (0-1000)
   */
  int getLatencyMs() const;

  /**
   * @brief Get reconnection attempts count
   * @return Number of reconnection attempts
   */
  int getReconnectionAttempts() const;

  /**
   * @brief Get video track ID
   * @return Video track ID string
   */
  std::string getVideoTrackId() const;

  /**
   * @brief Get data channel ID
   * @return Data channel ID string
   */
  std::string getDataChannelId() const;

  /**
   * @brief Get state history
   * @return Vector of historical states
   */
  std::vector<SessionState> getStateHistory() const;

  /**
   * @brief Initiate connection to client
   * @param client_ip Client IP address
   * @return Success or error
   *
   * Transitions state from DISCONNECTED to CONNECTING.
   */
  Result<void> connect(const std::string& client_ip);

  /**
   * @brief Mark session as connected
   * @return Success or error
   *
   * Transitions state from CONNECTING to CONNECTED.
   * Sets connection timestamp.
   */
  Result<void> setConnected();

  /**
   * @brief Disconnect session
   *
   * Transitions state to DISCONNECTED.
   * Resets display source ID and timestamps.
   */
  void disconnect();

  /**
   * @brief Initiate reconnection
   * @return Success or error
   *
   * Transitions state from ERROR or CONNECTED to RECONNECTING.
   */
  Result<void> reconnect();

  /**
   * @brief Mark session as error state
   *
   * Transitions state to ERROR.
   */
  void setError();

  /**
   * @brief Set display source ID
   * @param display_id Display ID (0-3)
   * @return Success or error
   *
   * Validates display ID is in range [0, 3].
   */
  Result<void> setDisplaySourceId(int display_id);

  /**
   * @brief Update latency measurement
   * @param latency_ms Latency in milliseconds (clamped to 0-1000)
   */
  void updateLatency(int latency_ms);

  /**
   * @brief Increment reconnection attempts counter
   */
  void incrementReconnectionAttempts();

  /**
   * @brief Reset reconnection attempts counter
   */
  void resetReconnectionAttempts();

  /**
   * @brief Set video track ID
   * @param track_id Video track ID string
   */
  void setVideoTrackId(const std::string& track_id);

  /**
   * @brief Set data channel ID
   * @param channel_id Data channel ID string
   */
  void setDataChannelId(const std::string& channel_id);

  /**
   * @brief Update activity timestamp to current time
   *
   * Used for timeout detection.
   */
  void updateActivity();

  /**
   * @brief Initialize remote desktop server
   * @param config Server configuration
   * @return Success or error
   */
  Result<void> initializeDesktopServer(const DesktopServerConfig& config);

  /**
   * @brief Start remote desktop server
   * @return Success or error
   */
  Result<void> startDesktopServer();

  /**
   * @brief Stop remote desktop server
   */
  void stopDesktopServer();

  /**
   * @brief Check if desktop server is running
   * @return true if running, false otherwise
   */
  [[nodiscard]] bool isDesktopServerRunning() const noexcept;

  /**
   * @brief Get HTTP server URL
   * @return HTTP URL string
   */
  std::string getHttpUrl() const;

  /**
   * @brief Get signaling server URL
   * @return Signaling URL string
   */
  std::string getSignalingUrl() const;

private:
  std::string generateSessionId();
  bool isValidStateTransition(SessionState from, SessionState to);

  std::string session_id_;
  SessionState state_;
  std::string client_ip_;
  std::chrono::system_clock::time_point connected_at_;
  std::chrono::system_clock::time_point last_activity_;
  int display_source_id_;
  std::atomic<int> latency_ms_;
  std::atomic<int> reconnection_attempts_;
  std::string video_track_id_;
  std::string data_channel_id_;
  std::vector<SessionState> state_history_;

  // Remote desktop server (PIMPL pattern to avoid circular dependency)
  class RemoteDesktopServerImpl;
  std::unique_ptr<RemoteDesktopServerImpl> desktop_server_;

  mutable std::mutex mutex_;
};

} // namespace screensdk
