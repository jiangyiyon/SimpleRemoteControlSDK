/**
 * @file remote_desktop_server.h
 * @brief Remote desktop server coordinator
 *
 * This class coordinates all remote desktop components including
 * HTTP server, signaling server, screen capture, encoder, and WebRTC transport.
 *
 * Usage:
 *   RemoteDesktopServer server;
 *   ServerConfig config;
 *   config.http_port = 8080;
 *   config.signaling_port = 8081;
 *
 *   server.initialize(config);
 *   server.start();
 *   // ... server is running ...
 *   server.stop();
 *   server.shutdown();
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "screensdk/capture/i_screen_capture.h"
#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/transport/i_webrtc_transport.h"
#include "screensdk/transport/video_source.h"
#include "screensdk/utils/types.h"
#include "screensdk/utils/error.h"
#include "screensdk/server/signaling_server.h"

namespace screensdk {

// Forward declarations
struct IHttpServer;

} // namespace screensdk

namespace screensdk {
namespace server {
class SignalingServer;
} // namespace server
} // namespace screensdk

namespace screensdk {

/**
 * @brief Server configuration
 */
struct ServerConfig {
    int http_port{8080};
    int signaling_port{8081};
    std::string web_root{"web"};
    int display_id{1};
    int fps{30};
    int max_bitrate_bps{15000000};
    std::string stun_server{"stun:stun.l.google.com:19302"};
};

/**
 * @brief Remote desktop server coordinator
 *
 * Coordinates HTTP server, signaling server, screen capture,
 * encoder, and WebRTC transport for remote desktop streaming.
 */
class RemoteDesktopServer : public server::ISignalingCallback {
public:
    /**
     * @brief Constructor
     */
    RemoteDesktopServer();

    /**
     * @brief Destructor - automatically shutdown if running
     */
    ~RemoteDesktopServer();

    // Disable copy and move
    RemoteDesktopServer(const RemoteDesktopServer&) = delete;
    RemoteDesktopServer& operator=(const RemoteDesktopServer&) = delete;
    RemoteDesktopServer(RemoteDesktopServer&&) = delete;
    RemoteDesktopServer& operator=(RemoteDesktopServer&&) = delete;

    /**
     * @brief Initialize server with configuration
     *
     * Creates and initializes all sub-components:
     * - HttpServer for static file serving
     * - SignalingServer for WebRTC signaling
     * - IScreenCapture for screen capture
     * - IVideoEncoder for video encoding
     * - IWebrtcTransport for WebRTC transport
     *
     * @param config Server configuration
     * @return Result<void> Success if initialization succeeded
     */
    Result<void> initialize(const ServerConfig& config);

    /**
     * @brief Shutdown server and release all resources
     *
     * This method is safe to call multiple times.
     * It will stop all running services and release all resources.
     */
    void shutdown();

    /**
     * @brief Start all server services
     *
     * Starts HTTP server, signaling server, and frame capture thread.
     *
     * @return Result<void> Success if all services started successfully
     */
    Result<void> start();

    /**
     * @brief Stop all server services
     *
     * Stops frame capture thread, HTTP server, and signaling server.
     * This method is thread-safe and can be called from any thread.
     */
    void stop();

    /**
     * @brief Check if server is currently running
     *
     * @return true if server is running, false otherwise
     */
    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Set web root directory for static files
     *
     * @param path Web root directory path
     */
    void setWebRootDirectory(const std::string& path);

    /**
     * @brief Set HTTP server port
     *
     * @param port HTTP port number
     */
    void setHttpPort(int port);

    /**
     * @brief Set signaling server port
     *
     * @param port Signaling port number
     */
    void setSignalingPort(int port);

    /**
     * @brief Get HTTP server URL
     *
     * @return HTTP URL string (e.g., "http://localhost:8080")
     */
    std::string getHttpUrl() const;

    /**
     * @brief Get signaling server URL
     *
     * @return Signaling URL string (e.g., "http://localhost:8081")
     */
    std::string getSignalingUrl() const;

    /**
     * @brief Set state change callback
     *
     * @param callback Function called on state changes
     */
    void setStateChangeCallback(std::function<void(SessionState)> callback);

    /**
     * @brief Set error callback
     *
     * @param callback Function called on errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback);

    // ISignalingCallback interface implementation
    std::string onOfferReceived(const std::string& client_id,
                                const std::string& sdp) override;
    void onIceCandidateReceived(const std::string& client_id,
                               const IceCandidate& candidate) override;
    void onClientDisconnected(const std::string& client_id) override;

private:
    // WebRTC callback handlers
    void onWebrtcStateChange(ConnectionState state);
    void onIceCandidate(const IceCandidate& candidate);
    void onWebrtcError(const std::string& error);

    // Frame capture loop
    void captureLoop();

private:
    // Sub-components
    IHttpServer* http_server_{nullptr};
    std::unique_ptr<server::SignalingServer> signaling_server_;

    // Components created via factory functions (must be manually released)
    IScreenCapture* screen_capture_{nullptr};
    IEncoderFactory* encoder_factory_{nullptr};
    IVideoEncoder* video_encoder_{nullptr};
    IWebrtcTransport* webrtc_transport_{nullptr};
    IVideoSource* video_source_{nullptr};

    // Configuration
    ServerConfig config_;
    std::atomic<bool> running_{false};

    // Frame capture thread
    std::jthread capture_thread_;

    // Callbacks
    std::function<void(SessionState)> state_callback_;
    std::function<void(const std::string&)> error_callback_;
    mutable std::mutex callback_mutex_;  // Protect callback variables

    // Mutex for thread safety
    mutable std::mutex mutex_;

    // Current client ID for signaling
    std::string current_client_id_;
};

} // namespace screensdk
