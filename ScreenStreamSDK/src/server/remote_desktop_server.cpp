/**
 * @file remote_desktop_server.cpp
 * @brief Remote desktop server coordinator implementation
 */

#include "screensdk/server/remote_desktop_server.h"
#include "screensdk/server/http_server.h"
#include "screensdk/server/signaling_server.h"
#include "screensdk/core/session.h"
#include "screensdk/capture/screen_capture_video_source.h"

#include <iostream>
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
    signaling_server_->setSignalingCallback(this);

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
    transport_config.force_media_transport = true;  // Required for sending tracks

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

    // Create video source and initialize video track
    // This ensures the track is available before receiving any offer
    std::cout << "[RemoteDesktopServer] Creating video source..." << std::endl;
    video_source_ = CreateVideoSourceFromScreenCapture(screen_capture_);
    if (!video_source_) {
        std::cerr << "[RemoteDesktopServer] Failed to create video source" << std::endl;
        // Continue without video source - it's optional for setup
    } else {
        std::cout << "[RemoteDesktopServer] Video source created, initializing..." << std::endl;
        if (!video_source_->init()) {
            std::cerr << "[RemoteDesktopServer] Failed to initialize video source" << std::endl;
            DestroyVideoSource(video_source_);
            video_source_ = nullptr;
        } else {
            std::cout << "[RemoteDesktopServer] Video source initialized" << std::endl;

            // Start video track (this will add the track to PeerConnection)
            // The track will be opened when DTLS-SRTP handshake completes
            std::cout << "[RemoteDesktopServer] Adding video track to PeerConnection..." << std::endl;
            webrtc_transport_->startVideoTrack(video_source_);
            std::cout << "[RemoteDesktopServer] Video track added successfully" << std::endl;
        }
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

    // Destroy video source
    if (video_source_) {
        video_source_->stop();
        DestroyVideoSource(video_source_);
        video_source_ = nullptr;
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
    oss << "ws://localhost:" << config_.signaling_port;
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
    std::cout << "[RemoteDesktopServer] WebRTC state changed to: "
              << static_cast<int>(state) << std::endl;

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

            std::cout << "[RemoteDesktopServer] ========== Connection ESTABLISHED ==========" << std::endl;
            std::cout << "[RemoteDesktopServer] Connection established" << std::endl;

            // Check if we have a video track (should have been created in onOfferReceived)
            if (video_source_) {
                std::cout << "[RemoteDesktopServer] Video source exists" << std::endl;
            } else {
                std::cerr << "[RemoteDesktopServer] Video source is nullptr!" << std::endl;
            }

            std::cout << "[RemoteDesktopServer] ==========================================" << std::endl;
            break;
        case ConnectionState::kFailed:
            session_state = screensdk::SessionState::kError;
            break;
        case ConnectionState::kDisconnected:
        case ConnectionState::kClosed:
            session_state = screensdk::SessionState::kDisconnected;

            // Stop video track when connection is closed
            std::cout << "[RemoteDesktopServer] Connection closed, stopping video track..." << std::endl;
            if (webrtc_transport_) {
                webrtc_transport_->stopVideoTrack();
            }
            if (video_source_) {
                video_source_->stop();
                DestroyVideoSource(video_source_);
                video_source_ = nullptr;
            }
            std::cout << "[RemoteDesktopServer] Video track stopped" << std::endl;
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
    std::lock_guard<std::mutex> lock(mutex_);

    if (!current_client_id_.empty() && signaling_server_) {
        signaling_server_->sendIceCandidate(current_client_id_, candidate);
    }
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

std::string RemoteDesktopServer::onOfferReceived(const std::string& client_id,
                                                   const std::string& sdp) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "[RemoteDesktopServer] Received offer from client: " << client_id
              << ", SDP length: " << sdp.length() << std::endl;

    current_client_id_ = client_id;

    std::cout << "[RemoteDesktopServer] Setting remote description..." << std::endl;
    auto set_result = webrtc_transport_->setRemoteDescription(sdp, SdpType::kOffer);
    if (!set_result) {
        std::cerr << "[RemoteDesktopServer] Failed to set remote description: "
                  << set_result.error().message << std::endl;
        return "";
    }

    std::cout << "[RemoteDesktopServer] Remote description set successfully" << std::endl;

    // Check connection state before creating answer
    auto conn_state = webrtc_transport_->getConnectionState();
    std::cout << "[RemoteDesktopServer] Connection state before createAnswer: "
              << static_cast<int>(conn_state) << std::endl;

    // Create answer to respond to client's offer
    std::cout << "[RemoteDesktopServer] Creating answer..." << std::endl;
    auto answer_result = webrtc_transport_->createAnswer();
    if (!answer_result) {
        std::cerr << "[RemoteDesktopServer] Failed to create answer: "
                  << answer_result.error().message << std::endl;
        return "";
    }

    std::string answer = answer_result.value();
    std::cout << "[RemoteDesktopServer] Created answer, length: " << answer.length() << std::endl;

    return answer;
}

void RemoteDesktopServer::onIceCandidateReceived(const std::string& client_id,
                                                  const IceCandidate& candidate) {
    std::cout << "[RemoteDesktopServer] Received ICE candidate from client: " << client_id
              << ", candidate: " << candidate.candidate.substr(0, 50) << "..." << std::endl;

    auto conn_state = webrtc_transport_->getConnectionState();
    std::cout << "[RemoteDesktopServer] Connection state before adding ICE candidate: "
              << static_cast<int>(conn_state) << std::endl;

    auto result = webrtc_transport_->addIceCandidate(candidate);
    if (!result) {
        std::cerr << "[RemoteDesktopServer] Failed to add ICE candidate: "
                  << result.error().message << std::endl;
    } else {
        std::cout << "[RemoteDesktopServer] ICE candidate added successfully" << std::endl;
    }
}

void RemoteDesktopServer::onClientDisconnected(const std::string& client_id) {
    std::cout << "[RemoteDesktopServer] Client disconnected: " << client_id << std::endl;

    std::lock_guard<std::mutex> lock(mutex_);

    if (current_client_id_ == client_id) {
        current_client_id_.clear();
    }
}

} // namespace screensdk
