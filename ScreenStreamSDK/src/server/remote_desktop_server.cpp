/**
 * @file remote_desktop_server.cpp
 * @brief Remote desktop server coordinator implementation
 */

#include "screensdk/server/remote_desktop_server.h"
#include "screensdk/server/http_server.h"
#include "screensdk/server/signaling_server.h"
#include "screensdk/core/session.h"

#include <sstream>

namespace screensdk {

RemoteDesktopServer::RemoteDesktopServer() = default;

RemoteDesktopServer::~RemoteDesktopServer() {
    shutdown();
}

Result<void> RemoteDesktopServer::initialize(const ServerConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Server already initialized");
    }

    // Validate port numbers
    if (config.http_port <= 0 || config.http_port > 65535) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Invalid HTTP port number: " + std::to_string(config.http_port));
    }
    if (config.signaling_port <= 0 || config.signaling_port > 65535) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Invalid signaling port number: " + std::to_string(config.signaling_port));
    }

    config_ = config;

    // Create HTTP server
    http_server_ = CreateHttpServer();
    if (!http_server_) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to create HTTP server");
    }
    auto set_root_result = http_server_->setRootDirectory(config_.web_root);
    if (!set_root_result) {
        DestroyHttpServer(http_server_);
        http_server_ = nullptr;
        return set_root_result;
    }

    // Create signaling server
    signaling_server_ = std::make_unique<screensdk::server::SignalingServer>();

    // Create screen capture
    screen_capture_ = CreateScreenCapture();
    if (!screen_capture_) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to create screen capture");
    }

    // Initialize screen capture
    if (!screen_capture_->initialize(config_.display_id)) {
        DestroyScreenCapture(screen_capture_);
        screen_capture_ = nullptr;
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to initialize screen capture");
    }

    // Create encoder factory
    encoder_factory_ = CreateEncoderFactory();
    if (!encoder_factory_) {
        DestroyScreenCapture(screen_capture_);
        screen_capture_ = nullptr;
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to create encoder factory");
    }

    // Create video encoder
    video_encoder_ = encoder_factory_->createEncoder();
    if (!video_encoder_) {
        DestroyScreenCapture(screen_capture_);
        screen_capture_ = nullptr;
        DestroyEncoderFactory(encoder_factory_);
        encoder_factory_ = nullptr;
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to create video encoder");
    }

    // Create WebRTC transport
    webrtc_transport_ = CreateWebrtcTransport();
    if (!webrtc_transport_) {
        DestroyScreenCapture(screen_capture_);
        screen_capture_ = nullptr;
        DestroyEncoderFactory(encoder_factory_);
        encoder_factory_ = nullptr;
        // Note: encoder is owned by factory, don't destroy it
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to create WebRTC transport");
    }

    // Set WebRTC callbacks
    webrtc_transport_->setStateChangeCallback(
        [this](ConnectionState state) { onWebrtcStateChange(state); });
    webrtc_transport_->setIceCandidateCallback(
        [this](const IceCandidate& candidate) { onIceCandidate(candidate); });
    webrtc_transport_->setErrorCallback(
        [this](const std::string& error) { onWebrtcError(error); });

    // Configure WebRTC transport
    TransportConfig transport_config;
    transport_config.stun_server = config_.stun_server;
    transport_config.max_bitrate_bps = config_.max_bitrate_bps;

    if (!webrtc_transport_->initialize(transport_config)) {
        DestroyScreenCapture(screen_capture_);
        screen_capture_ = nullptr;
        DestroyEncoderFactory(encoder_factory_);
        encoder_factory_ = nullptr;
        DestroyWebrtcTransport(webrtc_transport_);
        webrtc_transport_ = nullptr;
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to initialize WebRTC transport");
    }

    return Result<void>::make_ok();
}

void RemoteDesktopServer::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Stop if running
    running_ = false;

    // Destroy WebRTC transport
    if (webrtc_transport_) {
        webrtc_transport_->shutdown();
        DestroyWebrtcTransport(webrtc_transport_);
        webrtc_transport_ = nullptr;
    }

    // Destroy screen capture
    if (screen_capture_) {
        screen_capture_->shutdown();
        DestroyScreenCapture(screen_capture_);
        screen_capture_ = nullptr;
    }

    // Destroy encoder factory
    if (encoder_factory_) {
        DestroyEncoderFactory(encoder_factory_);
        encoder_factory_ = nullptr;
    }

    video_encoder_ = nullptr;

    // Destroy servers
    if (http_server_) {
        if (http_server_->isRunning()) {
            http_server_->stop();
        }
        DestroyHttpServer(http_server_);
        http_server_ = nullptr;
    }
    signaling_server_.reset();
}

Result<void> RemoteDesktopServer::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Server already running");
    }

    if (!http_server_ || !signaling_server_) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Server not initialized");
    }

    // Start HTTP server
    auto http_result = http_server_->start(config_.http_port);
    if (!http_result) {
        return http_result;
    }

    // Start signaling server
    auto sig_result = signaling_server_->start(config_.signaling_port);
    if (!sig_result) {
        http_server_->stop();
        return sig_result;
    }

    // Start screen capture
    auto displays = screen_capture_->enumerateDisplays();
    if (displays.empty()) {
        http_server_->stop();
        signaling_server_->stop();
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "No displays available");
    }

    // Convert 1-based display_id to 0-based index
    int display_index = config_.display_id - 1;
    if (display_index < 0 || static_cast<size_t>(display_index) >= displays.size()) {
        http_server_->stop();
        signaling_server_->stop();
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Invalid display ID: " + std::to_string(config_.display_id));
    }

    if (!screen_capture_->startCapture(displays[display_index])) {
        http_server_->stop();
        signaling_server_->stop();
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Failed to start screen capture");
    }

    // Start capture thread
    running_ = true;
    capture_thread_ = std::jthread([this](std::stop_token token) {
        captureLoop();
    });

    return Result<void>::make_ok();
}

void RemoteDesktopServer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return;
    }

    // Stop capture thread
    running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.request_stop();
        capture_thread_.join();
    }

    // Stop screen capture
    if (screen_capture_) {
        screen_capture_->stopCapture();
    }

    // Stop signaling server
    if (signaling_server_) {
        signaling_server_->stop();
    }

    // Stop HTTP server
    if (http_server_) {
        http_server_->stop();
    }

    running_ = false;
}

bool RemoteDesktopServer::isRunning() const noexcept {
    return running_.load();
}

void RemoteDesktopServer::setWebRootDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.web_root = path;
    if (http_server_) {
        auto result = http_server_->setRootDirectory(path);
        (void)result; // Ignore result for backward compatibility
    }
}

void RemoteDesktopServer::setHttpPort(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.http_port = port;
}

void RemoteDesktopServer::setSignalingPort(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.signaling_port = port;
}

std::string RemoteDesktopServer::getHttpUrl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "http://localhost:" << config_.http_port;
    return oss.str();
}

std::string RemoteDesktopServer::getSignalingUrl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "http://localhost:" << config_.signaling_port;
    return oss.str();
}

void RemoteDesktopServer::setStateChangeCallback(
    std::function<void(SessionState)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    state_callback_ = callback;
}

void RemoteDesktopServer::setErrorCallback(
    std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    error_callback_ = callback;
}

void RemoteDesktopServer::onWebrtcStateChange(ConnectionState state) {
    // No need to lock mutex_ here, only protect callback access
    screensdk::SessionState session_state;
    switch (state) {
        case ConnectionState::kNew:
        case ConnectionState::kChecking:
            session_state = screensdk::SessionState::kConnecting;
            break;
        case ConnectionState::kConnected:
        case ConnectionState::kCompleted:
            session_state = screensdk::SessionState::kConnected;
            break;
        case ConnectionState::kFailed:
            session_state = screensdk::SessionState::kError;
            break;
        case ConnectionState::kDisconnected:
        case ConnectionState::kClosed:
            session_state = screensdk::SessionState::kDisconnected;
            break;
        default:
            session_state = screensdk::SessionState::kDisconnected;
            break;
    }

    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (state_callback_) {
        state_callback_(session_state);
    }
}

void RemoteDesktopServer::onIceCandidate(const IceCandidate& candidate) {
    // TODO: Forward ICE candidates via signaling server
    // This will be implemented in Phase 5
}

void RemoteDesktopServer::onWebrtcError(const std::string& error) {
    std::lock_guard<std::mutex> lock(callback_mutex_);

    if (error_callback_) {
        error_callback_(error);
    }
}

void RemoteDesktopServer::captureLoop() {
    while (running_) {
        // Get next frame from screen capture
        auto frame = screen_capture_->getNextFrame(0);
        if (!frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // TODO: Encode frame and send via WebRTC
        // This will be implemented in Phase 5
        // For now, just simulate processing
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace screensdk
