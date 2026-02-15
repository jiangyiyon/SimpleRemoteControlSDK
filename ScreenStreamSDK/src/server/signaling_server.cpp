/**
 * @file signaling_server.cpp
 * @brief WebSocket signaling server implementation
 */

// Define before including any Windows headers to avoid conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "screensdk/server/signaling_server.h"
#include "screensdk/utils/error.h"
#include <httplib.h>
#include <json.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace screensdk::server {

namespace {

/**
 * @brief Generate a random client ID
 *
 * Format: client_<timestamp>_<random>
 */
std::string generateRandomClientId(uint64_t counter) {
    // Use counter and timestamp for uniqueness
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

/**
 * @brief Create JSON message from SignalMessage base
 */
json signalMessageToJson(const SignalMessage& msg) {
    json j;
    j["type"] = msg.type;
    j["client_id"] = msg.client_id;
    j["target_client_id"] = msg.target_client_id;
    return j;
}

} // anonymous namespace

/**
 * @brief PIMPL implementation class
 */
class SignalingServer::Impl {
public:
    Impl() {
        server_ = std::make_unique<httplib::Server>();
        setupRoutes();
    }

    ~Impl() = default;

    // Disable copy and move
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    Result<void> start(int port) {
        std::lock_guard<std::mutex> lock(server_mutex_);

        if (running_.load()) {
            return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                            "Server is already running");
        }

        if (port <= 0 || port > 65535) {
            return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                            "Invalid port number: " + std::to_string(port));
        }

        // Start server in a separate thread
        running_.store(true);
        server_thread_ = std::jthread(&Impl::serverThreadFunc, this, port);

        // Give server a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        return Result<void>::make_ok();
    }

    void stop() {
        std::lock_guard<std::mutex> lock(server_mutex_);

        if (!running_.load()) {
            return;
        }

        running_.store(false);

        // Stop the server
        if (server_) {
            server_->stop();
        }

        // jthread will automatically join when destroyed
        server_thread_.request_stop();
    }

    [[nodiscard]] bool isRunning() const noexcept {
        return running_.load();
    }

    [[nodiscard]] size_t getClientCount() const noexcept {
        return client_count_.load();
    }

private:
    void setupRoutes() {
        // Add health check endpoint
        server_->Get("/health", [](const httplib::Request& /*req*/, httplib::Response& res) {
            res.set_content(R"({"status":"ok","server":"SignalingServer"})", "application/json");
        });

        // Add status endpoint
        server_->Get("/status", [](const httplib::Request& /*req*/, httplib::Response& res) {
            res.set_content(R"({"status":"running","service":"signaling-server"})",
                           "application/json");
        });

        // Add signaling endpoint (POST for SDP exchange)
        server_->Post("/signal", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json j = json::parse(req.body);
                std::string type = j.value("type", "");

                std::cout << "[SignalingServer] Received message type: " << type << std::endl;

                // Process signaling message
                // For now, just echo back
                res.set_content(R"({"status":"ok","message":"received"})", "application/json");

            } catch (const json::exception& e) {
                std::cerr << "[SignalingServer] Failed to parse message: " << e.what() << std::endl;
                res.status = 400;
                res.set_content(R"({"status":"error","message":"invalid json"})", "application/json");
            }
        });
    }

    void serverThreadFunc(int port) {
        try {
            server_->listen("0.0.0.0", port);
        } catch (const std::exception& e) {
            std::cerr << "[SignalingServer] Error: " << e.what() << std::endl;
            running_.store(false);
        }
    }

private:
    std::unique_ptr<httplib::Server> server_;
    std::atomic<bool> running_{false};
    std::atomic<size_t> client_count_{0};
    std::jthread server_thread_;
    std::mutex server_mutex_;
};

// Public interface - forward to implementation

SignalingServer::SignalingServer()
    : impl_(std::make_unique<Impl>()) {
}

SignalingServer::~SignalingServer() = default;

Result<void> SignalingServer::start(int port) {
    return impl_->start(port);
}

void SignalingServer::stop() {
    impl_->stop();
}

bool SignalingServer::isRunning() const noexcept {
    return impl_->isRunning();
}

size_t SignalingServer::getClientCount() const noexcept {
    return impl_->getClientCount();
}

} // namespace screensdk::server
