/**
 * @file screensdk.cpp
 * @brief Public API implementation for ScreenStreamSDK
 *
 * T047: Implement screensdk::createSession() public API
 *
 * Implements factory functions and public API for remote desktop session management.
 * Provides C-compatible extern "C" functions for cross-language interoperability.
 */

#include "screensdk/screensdk.h"
#include "screensdk/core/session.h"

#include <memory>
#include <string>

namespace screensdk {

// Session implementation
class SessionImpl : public ISession {
public:
  SessionImpl() : session_(std::make_unique<Session>()) {}

  std::string getSessionId() const override {
    return session_->getSessionId();
  }

  int getState() const override {
    return static_cast<int>(session_->getState());
  }

  std::string getClientIp() const override {
    return session_->getClientIp();
  }

  int getDisplaySourceId() const override {
    return session_->getDisplaySourceId();
  }

  int getLatencyMs() const override {
    return session_->getLatencyMs();
  }

  int getReconnectionAttempts() const override {
    return session_->getReconnectionAttempts();
  }

  std::vector<int> getStateHistory() const override {
    auto history = session_->getStateHistory();
    std::vector<int> result;
    result.reserve(history.size());
    for (const auto& state : history) {
      result.push_back(static_cast<int>(state));
    }
    return result;
  }

  int connect(const std::string& client_ip) override {
    auto result = session_->connect(client_ip);
    return result ? 0 : -1;
  }

  int setConnected() override {
    auto result = session_->setConnected();
    return result ? 0 : -1;
  }

  void disconnect() override {
    session_->disconnect();
  }

  int reconnect() override {
    auto result = session_->reconnect();
    return result ? 0 : -1;
  }

  void setError() override {
    session_->setError();
  }

  int setDisplaySourceId(int display_id) override {
    auto result = session_->setDisplaySourceId(display_id);
    return result ? 0 : -1;
  }

  void updateLatency(int latency_ms) override {
    session_->updateLatency(latency_ms);
  }

  void incrementReconnectionAttempts() override {
    session_->incrementReconnectionAttempts();
  }

  void resetReconnectionAttempts() override {
    session_->resetReconnectionAttempts();
  }

private:
  std::unique_ptr<Session> session_;
};

} // namespace screensdk

// Exported C functions

extern "C" {

/**
 * @brief Create a new session
 */
screensdk::ISession* screensdk::CreateSession() {
  try {
    return new screensdk::SessionImpl();
  } catch (const std::exception& e) {
    return nullptr;
  }
}

/**
 * @brief Destroy a session
 */
void screensdk::DestroySession(ISession* session) {
  if (session != nullptr) {
    delete session;
  }
}

/**
 * @brief Get SDK version
 */
const char* screensdk::GetSDKVersion() {
  return "1.0.0";
}

/**
 * @brief Get SDK build information
 */
const char* screensdk::GetSDKBuildInfo() {
  return "Build: 2026-02-14";
}

} // extern "C"
