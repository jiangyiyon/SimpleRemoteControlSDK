/**
 * @file signaling_server.h
 * @brief WebSocket signaling server for WebRTC peer connection
 *
 * This class provides a WebSocket-based signaling server for WebRTC
 * peer connections. It handles SDP offer/answer exchange and ICE
 * candidate forwarding between clients.
 *
 * Uses libdatachannel WebSocket for real-time bidirectional communication.
 *
 * Message Protocol:
 * - All messages are JSON formatted
 * - Types: "offer", "answer", "ice-candidate", "join", "leave", "error"
 */

#pragma once

// Define before including any Windows headers to avoid conflicts
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <memory>
#include <thread>
#include <map>

#include "screensdk/utils/error.h"
#include "screensdk/transport/i_webrtc_transport.h"

// Forward declaration to avoid including rtc headers
namespace rtc {
class WebSocketServer;
class WebSocket;
}

namespace screensdk {

namespace server {

/**
 * @brief Callback interface for RemoteDesktopServer to handle signaling messages
 *
 * RemoteDesktopServer implements this interface to receive offers and
 * ICE candidates from clients, and send back answers and ICE candidates.
 */
class ISignalingCallback {
public:
    virtual ~ISignalingCallback() = default;

    /**
     * @brief Called when a client sends SDP offer
     * @param client_id Client identifier
     * @param sdp SDP offer string
     * @return SDP answer string, or empty string on error
     */
    virtual std::string onOfferReceived(const std::string& client_id,
                                        const std::string& sdp) = 0;

    /**
     * @brief Called when a client sends ICE candidate
     * @param client_id Client identifier
     * @param candidate ICE candidate
     */
    virtual void onIceCandidateReceived(const std::string& client_id,
                                        const IceCandidate& candidate) = 0;

    /**
     * @brief Called when a client disconnects
     * @param client_id Client identifier
     */
    virtual void onClientDisconnected(const std::string& client_id) = 0;
};

/**
 * @brief Base message structure for signaling
 */
struct SignalMessage {
    std::string type;
    std::string client_id;
};

/**
 * @brief SDP offer message
 */
struct OfferMessage : SignalMessage {
    std::string sdp;
};

/**
 * @brief SDP answer message
 */
struct AnswerMessage : SignalMessage {
    std::string sdp;
};

/**
 * @brief ICE candidate message
 */
struct IceCandidateMessage : SignalMessage {
    std::string candidate;
    std::string sdp_mid;
    int sdp_mline_index = 0;
};

/**
 * @brief Join message (sent by server when client connects)
 */
struct JoinMessage : SignalMessage {
    std::string client_id;
};

/**
 * @brief Leave message (sent by server when client disconnects)
 */
struct LeaveMessage : SignalMessage {
    std::string client_id;
};

/**
 * @brief Error message
 */
struct ErrorMessage : SignalMessage {
    std::string error_code;
    std::string error_message;
};

/**
 * @brief Signaling server for WebRTC peer connections
 *
 * Provides a WebSocket-based signaling server for WebRTC peer connections.
 * Handles SDP offer/answer exchange and ICE candidate forwarding between clients.
 *
 * Uses libdatachannel WebSocket for real-time bidirectional communication.
 *
 * Usage:
 *   auto server = std::make_unique<SignalingServer>();
 *   server->setSignalingCallback(&remote_desktop_server);
 *   server->start(8080);
 *   // ... server is running ...
 *   server->stop();
 *
 * Message Flow:
 * 1. Client connects to server via WebSocket
 * 2. Server assigns unique client_id and sends join message
 * 3. Client sends offer, server forwards to RemoteDesktopServer
 * 4. RemoteDesktopServer creates answer, server sends back to client
 * 5. Clients exchange ICE candidates via server
 * 6. Server sends leave message when client disconnects
 */
class SignalingServer {
public:
    /**
     * @brief Constructor
     */
    SignalingServer();

    /**
     * @brief Destructor - automatically stops server if running
     */
    ~SignalingServer();

    // Disable copy and move
    SignalingServer(const SignalingServer&) = delete;
    SignalingServer& operator=(const SignalingServer&) = delete;
    SignalingServer(SignalingServer&&) = delete;
    SignalingServer& operator=(SignalingServer&&) = delete;

    /**
     * @brief Set signaling callback for handling offers and ICE candidates
     * @param callback Pointer to callback interface (must remain valid)
     */
    void setSignalingCallback(ISignalingCallback* callback);

    /**
     * @brief Start the signaling server on specified port
     * @param port Port number to listen on (1-65535)
     * @return Result<void> Success if server started successfully
     */
    Result<void> start(int port);

    /**
     * @brief Stop the signaling server
     *
     * This method is thread-safe and can be called from any thread.
     * It will block until the server is fully stopped.
     */
    void stop();

    /**
     * @brief Check if the server is currently running
     * @return true if server is running, false otherwise
     */
    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Get the number of connected clients
     * @return Number of active connections
     */
    [[nodiscard]] size_t getClientCount() const noexcept;

    /**
     * @brief Send answer to a specific client
     * @param client_id Client identifier
     * @param sdp SDP answer string
     * @return Result<void> Success if message sent successfully
     */
    Result<void> sendAnswer(const std::string& client_id, const std::string& sdp);

    /**
     * @brief Send ICE candidate to a specific client
     * @param client_id Client identifier
     * @param candidate ICE candidate
     * @return Result<void> Success if message sent successfully
     */
    Result<void> sendIceCandidate(const std::string& client_id,
                                   const IceCandidate& candidate);

private:
    void onClientConnected(std::shared_ptr<rtc::WebSocket> client);
    void onClientMessage(std::shared_ptr<rtc::WebSocket> client,
                         const std::string& message);
    void onClientDisconnected(std::shared_ptr<rtc::WebSocket> client,
                              const std::string& client_id);
    std::string generateClientId();
    void handleOffer(const std::string& client_id, const std::string& sdp);
    void handleIceCandidate(const std::string& client_id,
                            const IceCandidate& candidate);

private:
    std::unique_ptr<rtc::WebSocketServer> server_;
    std::atomic<bool> running_{false};
    std::atomic<size_t> client_count_{0};
    std::atomic<uint64_t> client_counter_{0};

    ISignalingCallback* signaling_callback_{nullptr};
    mutable std::mutex callback_mutex_;

    std::map<std::string, std::shared_ptr<rtc::WebSocket>> clients_;
    mutable std::mutex clients_mutex_;
};

} // namespace server

} // namespace screensdk
