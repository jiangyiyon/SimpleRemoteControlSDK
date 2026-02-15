/**
 * @file signaling_server.cpp
 * @brief WebSocket signaling server implementation
 */

// Define before including any Windows headers to avoid conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "screensdk/server/signaling_server.h"
#include "screensdk/utils/error.h"
#include <rtc/rtc.hpp>
#include <rtc/global.hpp>
#include <json.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace screensdk {

namespace server {

namespace {

/**
 * @brief Initialize libdatachannel once on first use
 */
class LibdatachannelInitializer {
public:
    LibdatachannelInitializer() {
        std::cout << "[SignalingServer] Initializing libdatachannel..." << std::endl;
        rtc::InitLogger(rtc::LogLevel::Warning);
        rtc::Preload();
        std::cout << "[SignalingServer] libdatachannel initialized" << std::endl;
    }
};

static LibdatachannelInitializer g_initializer;

} // anonymous namespace

SignalingServer::SignalingServer() = default;

SignalingServer::~SignalingServer() {
    stop();
}

void SignalingServer::setSignalingCallback(ISignalingCallback* callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    signaling_callback_ = callback;
}

Result<void> SignalingServer::start(int port) {
    if (running_.load()) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Server is already running");
    }

    if (port <= 0 || port > 65535) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Invalid port number: " + std::to_string(port));
    }

    try {
        rtc::WebSocketServerConfiguration config;
        config.port = static_cast<uint16_t>(port);
        config.enableTls = false;

        server_ = std::make_unique<rtc::WebSocketServer>(config);

        server_->onClient([this](std::shared_ptr<rtc::WebSocket> client) {
            onClientConnected(client);
        });

        running_.store(true);

        std::cout << "[SignalingServer] WebSocket server started on port " << port << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[SignalingServer] Failed to start: " << e.what() << std::endl;
        running_.store(false);
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Failed to start server: " + std::string(e.what()));
    }

    return Result<void>::make_ok();
}

void SignalingServer::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);

    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.clear();

    if (server_) {
        server_->stop();
    }

    std::cout << "[SignalingServer] WebSocket server stopped" << std::endl;
}

bool SignalingServer::isRunning() const noexcept {
    return running_.load();
}

size_t SignalingServer::getClientCount() const noexcept {
    return client_count_.load();
}

Result<void> SignalingServer::sendAnswer(const std::string& client_id, const std::string& sdp) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Client not found: " + client_id);
    }

    auto& client = it->second;
    if (!client || !client->isOpen()) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Client connection not open");
    }

    AnswerMessage answer_msg;
    answer_msg.type = "answer";
    answer_msg.client_id = "server";
    answer_msg.sdp = sdp;

    json j;
    j["type"] = answer_msg.type;
    j["client_id"] = answer_msg.client_id;
    j["sdp"] = answer_msg.sdp;

    std::string message_str = j.dump();

    if (!client->send(message_str)) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Failed to send answer message");
    }

    std::cout << "[SignalingServer] Sent answer to client: " << client_id << std::endl;

    return Result<void>::make_ok();
}

Result<void> SignalingServer::sendIceCandidate(const std::string& client_id,
                                               const IceCandidate& candidate) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Client not found: " + client_id);
    }

    auto& client = it->second;
    if (!client || !client->isOpen()) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Client connection not open");
    }

    IceCandidateMessage ice_msg;
    ice_msg.type = "ice-candidate";
    ice_msg.client_id = "server";
    ice_msg.candidate = candidate.candidate;
    ice_msg.sdp_mid = candidate.sdp_mid;
    ice_msg.sdp_mline_index = candidate.sdp_mline_index;

    json j;
    j["type"] = ice_msg.type;
    j["client_id"] = ice_msg.client_id;
    j["candidate"] = ice_msg.candidate;
    j["sdp_mid"] = ice_msg.sdp_mid;
    j["sdp_mline_index"] = ice_msg.sdp_mline_index;

    std::string message_str = j.dump();

    if (!client->send(message_str)) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                        "Failed to send ICE candidate message");
    }

    std::cout << "[SignalingServer] Sent ICE candidate to client: " << client_id << std::endl;

    return Result<void>::make_ok();
}

void SignalingServer::onClientConnected(std::shared_ptr<rtc::WebSocket> client) {
    std::string client_id = generateClientId();

    std::cout << "[SignalingServer] Client connected: " << client_id << std::endl;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_[client_id] = client;
        client_count_.store(clients_.size());
    }

    // Send join message
    JoinMessage join_msg;
    join_msg.type = "join";
    join_msg.client_id = client_id;

    json j;
    j["type"] = join_msg.type;
    j["client_id"] = join_msg.client_id;

    std::string message_str = j.dump();
    client->send(message_str);

    // Set up message handler
    client->onMessage([this, client, client_id](const rtc::message_variant& data) {
        if (std::holds_alternative<std::string>(data)) {
            onClientMessage(client, std::get<std::string>(data));
        }
    });

    // Set up close handler
    client->onClosed([this, client, client_id]() {
        onClientDisconnected(client, client_id);
    });

    // Set up error handler
    client->onError([this, client_id](const std::string& error) {
        std::cerr << "[SignalingServer] Client error (" << client_id << "): " << error << std::endl;
    });
}

void SignalingServer::onClientMessage(std::shared_ptr<rtc::WebSocket> client,
                                      const std::string& message) {
    try {
        json j = json::parse(message);
        std::string type = j.value("type", "");
        std::string client_id = j.value("client_id", "");

        std::cout << "[SignalingServer] Received message type: " << type
                  << " from client: " << client_id << std::endl;

        if (type == "offer") {
            std::string sdp = j.value("sdp", "");
            handleOffer(client_id, sdp);
        } else if (type == "ice-candidate") {
            IceCandidate candidate;
            candidate.candidate = j.value("candidate", "");
            candidate.sdp_mid = j.value("sdp_mid", "");
            candidate.sdp_mline_index = j.value("sdp_mline_index", 0);
            handleIceCandidate(client_id, candidate);
        } else {
            std::cerr << "[SignalingServer] Unknown message type: " << type << std::endl;
        }

    } catch (const json::exception& e) {
        std::cerr << "[SignalingServer] Failed to parse message: " << e.what() << std::endl;
    }
}

void SignalingServer::onClientDisconnected(std::shared_ptr<rtc::WebSocket> client,
                                           const std::string& client_id) {
    std::cout << "[SignalingServer] Client disconnected: " << client_id << std::endl;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.erase(client_id);
        client_count_.store(clients_.size());
    }

    // Notify callback
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (signaling_callback_) {
            signaling_callback_->onClientDisconnected(client_id);
        }
    }

    // Send leave message to client
    LeaveMessage leave_msg;
    leave_msg.type = "leave";
    leave_msg.client_id = client_id;

    json j;
    j["type"] = leave_msg.type;
    j["client_id"] = leave_msg.client_id;

    std::string message_str = j.dump();

    // Broadcast to all other clients
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& [id, ws] : clients_) {
            if (ws && ws->isOpen() && id != client_id) {
                ws->send(message_str);
            }
        }
    }
}

std::string SignalingServer::generateClientId() {
    uint64_t counter = client_counter_.fetch_add(1);
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    std::ostringstream oss;
    oss << "client_" << timestamp << "_" << counter << "_" << dis(gen);
    return oss.str();
}

void SignalingServer::handleOffer(const std::string& client_id, const std::string& sdp) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (signaling_callback_) {
        std::string answer = signaling_callback_->onOfferReceived(client_id, sdp);
        if (!answer.empty()) {
            sendAnswer(client_id, answer);
        }
    }
}

void SignalingServer::handleIceCandidate(const std::string& client_id,
                                         const IceCandidate& candidate) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (signaling_callback_) {
        signaling_callback_->onIceCandidateReceived(client_id, candidate);
    }
}

} // namespace server

} // namespace screensdk
