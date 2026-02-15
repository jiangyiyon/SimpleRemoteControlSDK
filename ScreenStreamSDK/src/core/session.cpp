#include "screensdk/core/session.h"
#include "screensdk/server/remote_desktop_server.h"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>

#undef max
#undef min

namespace screensdk {

// PIMPL implementation class
class Session::RemoteDesktopServerImpl {
public:
    RemoteDesktopServer server;
};

Session::Session()
    : session_id_(generateSessionId()),
      state_(SessionState::kDisconnected),
      client_ip_(),
      connected_at_(std::chrono::system_clock::now()),
      last_activity_(std::chrono::system_clock::now()),
      display_source_id_(-1),
      latency_ms_(0),
      reconnection_attempts_(0),
      video_track_id_(),
      data_channel_id_() {
  state_history_.push_back(SessionState::kDisconnected);
}

Session::~Session() {
  std::lock_guard<std::mutex> lock(mutex_);
  desktop_server_.reset();
}

std::string Session::getSessionId() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return session_id_;
}

SessionState Session::getState() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}

std::string Session::getClientIp() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return client_ip_;
}

std::chrono::system_clock::time_point Session::getConnectedAt() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return connected_at_;
}

std::chrono::system_clock::time_point Session::getLastActivity() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_activity_;
}

int Session::getDisplaySourceId() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return display_source_id_;
}

int Session::getLatencyMs() const {
  return latency_ms_.load();
}

int Session::getReconnectionAttempts() const {
  return reconnection_attempts_.load();
}

std::string Session::getVideoTrackId() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return video_track_id_;
}

std::string Session::getDataChannelId() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return data_channel_id_;
}

std::vector<SessionState> Session::getStateHistory() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_history_;
}

Result<void> Session::connect(const std::string& client_ip) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!isValidStateTransition(state_, SessionState::kConnecting)) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Invalid state transition");
  }

  state_ = SessionState::kConnecting;
  client_ip_ = client_ip;
  state_history_.push_back(SessionState::kConnecting);

  return Result<void>::make_ok();
}

Result<void> Session::setConnected() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!isValidStateTransition(state_, SessionState::kConnected)) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Invalid state transition");
  }

  state_ = SessionState::kConnected;
  connected_at_ = std::chrono::system_clock::now();
  last_activity_ = std::chrono::system_clock::now();
  state_history_.push_back(SessionState::kConnected);

  return Result<void>::make_ok();
}

void Session::disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);

  state_ = SessionState::kDisconnected;
  client_ip_.clear();
  connected_at_ = std::chrono::system_clock::now();
  display_source_id_ = -1;
  video_track_id_.clear();
  data_channel_id_.clear();
  reconnection_attempts_.store(0);
  latency_ms_.store(0);
  state_history_.push_back(SessionState::kDisconnected);
}

Result<void> Session::reconnect() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!isValidStateTransition(state_, SessionState::kReconnecting)) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Invalid state transition");
  }

  state_ = SessionState::kReconnecting;
  state_history_.push_back(SessionState::kReconnecting);

  return Result<void>::make_ok();
}

void Session::setError() {
  std::lock_guard<std::mutex> lock(mutex_);

  state_ = SessionState::kError;
  state_history_.push_back(SessionState::kError);
}

Result<void> Session::setDisplaySourceId(int display_id) {
  if (display_id < 0 || display_id > 3) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Invalid display source ID");
  }

  std::lock_guard<std::mutex> lock(mutex_);
  display_source_id_ = display_id;

  return Result<void>::make_ok();
}

void Session::updateLatency(int latency_ms) {
  // Clamp latency to valid range [0, 1000]
  latency_ms = std::max(0, std::min(1000, latency_ms));
  latency_ms_.store(latency_ms);
}

void Session::incrementReconnectionAttempts() {
  reconnection_attempts_.fetch_add(1);
}

void Session::resetReconnectionAttempts() {
  reconnection_attempts_.store(0);
}

void Session::setVideoTrackId(const std::string& track_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  video_track_id_ = track_id;
}

void Session::setDataChannelId(const std::string& channel_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  data_channel_id_ = channel_id;
}

void Session::updateActivity() {
  std::lock_guard<std::mutex> lock(mutex_);
  last_activity_ = std::chrono::system_clock::now();
}

Result<void> Session::initializeDesktopServer(const DesktopServerConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (desktop_server_) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Desktop server already initialized");
  }

  desktop_server_ = std::make_unique<RemoteDesktopServerImpl>();

  // Convert DesktopServerConfig to ServerConfig
  ServerConfig server_config;
  server_config.http_port = config.http_port;
  server_config.signaling_port = config.signaling_port;
  server_config.web_root = config.web_root;
  server_config.display_id = config.display_id;
  server_config.fps = config.fps;
  server_config.max_bitrate_bps = config.max_bitrate_bps;
  server_config.stun_server = config.stun_server;

  auto result = desktop_server_->server.initialize(server_config);

  if (!result) {
    desktop_server_.reset();
  }

  return result;
}

Result<void> Session::startDesktopServer() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!desktop_server_) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Desktop server not initialized");
  }

  return desktop_server_->server.start();
}

void Session::stopDesktopServer() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (desktop_server_) {
    desktop_server_->server.stop();
  }
}

bool Session::isDesktopServerRunning() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!desktop_server_) {
    return false;
  }

  return desktop_server_->server.isRunning();
}

std::string Session::getHttpUrl() const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!desktop_server_) {
    return "";
  }

  return desktop_server_->server.getHttpUrl();
}

std::string Session::getSignalingUrl() const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!desktop_server_) {
    return "";
  }

  return desktop_server_->server.getSignalingUrl();
}

std::string Session::generateSessionId() {
  // Generate UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (int i = 0; i < 36; ++i) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      oss << '-';
    } else if (i == 14) {
      oss << '4';  // Version 4
    } else if (i == 19) {
      int y = dis(gen) | 0x8;  // Variant 1 (10xx)
      oss << std::setw(1) << y;
    } else {
      oss << std::setw(1) << dis(gen);
    }
  }

  // Convert to uppercase
  std::string id = oss.str();
  std::transform(id.begin(), id.end(), id.begin(), ::toupper);

  return id;
}

bool Session::isValidStateTransition(SessionState from, SessionState to) {
  // Valid state transitions:
  // kDisconnected -> kConnecting
  // kConnecting -> kConnected
  // kConnecting -> kDisconnected
  // kConnected -> kDisconnected
  // kConnected -> kReconnecting (via setError then reconnect)
  // kReconnecting -> kConnected
  // kReconnecting -> kError
  // kError -> kReconnecting
  // kError -> kDisconnected

  switch (from) {
    case SessionState::kDisconnected:
      return to == SessionState::kConnecting;
    case SessionState::kConnecting:
      return to == SessionState::kConnected || to == SessionState::kDisconnected;
    case SessionState::kConnected:
      return to == SessionState::kDisconnected;
    case SessionState::kReconnecting:
      return to == SessionState::kConnected || to == SessionState::kError;
    case SessionState::kError:
      return to == SessionState::kReconnecting || to == SessionState::kDisconnected;
    default:
      return false;
  }
}

} // namespace screensdk
