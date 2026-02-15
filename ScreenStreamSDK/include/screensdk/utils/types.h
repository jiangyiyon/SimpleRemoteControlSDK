#pragma once

namespace screensdk {

/**
 * @brief Session state enumeration
 *
 * Represents the connection state of a remote desktop session.
 * State machine: kDisconnected -> kConnecting -> kConnected -> [kReconnecting*] -> kError/Reconnected
 */
enum class SessionState {
  kDisconnected,
  kConnecting,
  kConnected,
  kReconnecting,
  kError
};

} // namespace screensdk
