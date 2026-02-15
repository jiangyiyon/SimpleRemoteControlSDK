/**
 * @file signaling_server.h
 * @brief WebSocket signaling server for WebRTC peer connection
 *
 * This class provides a WebSocket-based signaling server for WebRTC
 * peer connections. It handles SDP offer/answer exchange and ICE
 * candidate forwarding between clients.
 *
 * Note: WebSocket support requires httplib v0.11.0 or higher.
 * Current implementation provides basic HTTP endpoints only.
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

#include "screensdk/utils/error.h"

// Forward declaration to avoid including httplib in header
namespace httplib {
class Server;
}

namespace screensdk {

namespace server {



/**
 * @brief Base message structure for signaling
 */
struct SignalMessage {
    std::string type;
    std::string client_id;
    std::string target_client_id;  // Empty for broadcast
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
    // Empty body, just type and client_id
};

/**
 * @brief Leave message (sent by server when client disconnects)
 */
struct LeaveMessage : SignalMessage {
    // Empty body, just type and client_id
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
 * Provides a signaling server for WebRTC peer connections.
 * Handles SDP offer/answer exchange and ICE candidate forwarding between clients.
 *
 * Note: Current implementation provides basic HTTP endpoints only.
 * WebSocket support requires httplib v0.11.0 or higher.
 *
 * Usage:
 *   auto server = std::make_unique<SignalingServer>();
 *   server->start(8080);
 *   // ... server is running ...
 *   server->stop();
 *
 * Message Flow:
 * 1. Client connects to server
 * 2. Server assigns unique client_id and sends join message
 * 3. Clients exchange SDP offers/answers via server
 * 4. Clients exchange ICE candidates via server
 * 5. Server broadcasts leave messages on disconnect
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
     * @brief Start the signaling server on specified port
     *
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
     *
     * @return true if server is running, false otherwise
     */
    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Get the number of connected clients
     *
     * @return Number of active connections
     */
    [[nodiscard]] size_t getClientCount() const noexcept;

private:
    void setupRoutes();
    void serverThreadFunc(int port);

private:
    std::unique_ptr<httplib::Server> server_;
    std::atomic<bool> running_{false};
    std::atomic<size_t> client_count_{0};
    std::jthread server_thread_;
    mutable std::mutex server_mutex_;
};

} // namespace server

} // namespace screensdk
