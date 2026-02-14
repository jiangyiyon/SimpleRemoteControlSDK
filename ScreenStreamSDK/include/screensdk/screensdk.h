#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "screensdk/export.h"

namespace screensdk {

// Forward declarations to avoid circular dependencies
class Session;

/**
 * @brief Session interface for remote desktop connection
 *
 * T047: Implement screensdk::createSession() public API
 *
 * Represents a remote desktop connection between Windows host and mobile client.
 * Provides a pure virtual interface for maximum ABI compatibility.
 */
struct SCREEN_STREAM_SDK_EXPORT ISession {
  virtual ~ISession() = default;

  /**
   * @brief Get session ID (UUID v4)
   * @return Session ID string
   */
  virtual std::string getSessionId() const = 0;

  /**
   * @brief Get current session state
   * @return Current state (0=Disconnected, 1=Connecting, 2=Connected, 3=Reconnecting, 4=Error)
   */
  virtual int getState() const = 0;

  /**
   * @brief Get client IP address
   * @return Client IP string, empty if not connected
   */
  virtual std::string getClientIp() const = 0;

  /**
   * @brief Get selected display source ID
   * @return Display ID (0-3), or -1 if not selected
   */
  virtual int getDisplaySourceId() const = 0;

  /**
   * @brief Get current latency in milliseconds
   * @return Latency in milliseconds (0-1000)
   */
  virtual int getLatencyMs() const = 0;

  /**
   * @brief Get reconnection attempts count
   * @return Number of reconnection attempts
   */
  virtual int getReconnectionAttempts() const = 0;

  /**
   * @brief Get state history
   * @return Vector of historical states (as integers)
   */
  virtual std::vector<int> getStateHistory() const = 0;

  /**
   * @brief Initiate connection to client
   * @param client_ip Client IP address
   * @return 0 on success, non-zero on error
   */
  virtual int connect(const std::string& client_ip) = 0;

  /**
   * @brief Mark session as connected
   * @return 0 on success, non-zero on error
   */
  virtual int setConnected() = 0;

  /**
   * @brief Disconnect session
   */
  virtual void disconnect() = 0;

  /**
   * @brief Initiate reconnection
   * @return 0 on success, non-zero on error
   */
  virtual int reconnect() = 0;

  /**
   * @brief Mark session as error state
   */
  virtual void setError() = 0;

  /**
   * @brief Set display source ID
   * @param display_id Display ID (0-3)
   * @return 0 on success, non-zero on error
   */
  virtual int setDisplaySourceId(int display_id) = 0;

  /**
   * @brief Update latency measurement
   * @param latency_ms Latency in milliseconds (clamped to 0-1000)
   */
  virtual void updateLatency(int latency_ms) = 0;

  /**
   * @brief Increment reconnection attempts counter
   */
  virtual void incrementReconnectionAttempts() = 0;

  /**
   * @brief Reset reconnection attempts counter
   */
  virtual void resetReconnectionAttempts() = 0;
};

/**
 * @brief Create a new session
 *
 * T047: Implement screensdk::createSession() public API
 *
 * Creates a new remote desktop session with unique ID and DISCONNECTED state.
 * The session must be destroyed using DestroySession() to avoid memory leaks.
 *
 * @return Session instance, or nullptr on failure
 */
extern "C" SCREEN_STREAM_SDK_EXPORT ISession* CreateSession();

/**
 * @brief Destroy a session
 * @param session Session instance to destroy (nullptr is safe)
 */
extern "C" SCREEN_STREAM_SDK_EXPORT void DestroySession(ISession* session);

/**
 * @brief Get SDK version
 * @return Version string in format "major.minor.patch"
 */
extern "C" SCREEN_STREAM_SDK_EXPORT const char* GetSDKVersion();

/**
 * @brief Get SDK build information
 * @return Build information string
 */
extern "C" SCREEN_STREAM_SDK_EXPORT const char* GetSDKBuildInfo();

} // namespace screensdk
