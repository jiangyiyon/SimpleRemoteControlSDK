/**
 * @file http_server.cpp
 * @brief HTTP static file server implementation
 */

// Define before including any Windows headers to avoid conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "http_server.h"
#include "screensdk/utils/error.h"
#include <httplib.h>
#include <filesystem>
#include <iostream>
#include <system_error>

// We need the full Response definition here
namespace httplib {
using Response = struct Response;
}

namespace screensdk::server {

HttpServer::HttpServer() {
    server_ = std::make_unique<httplib::Server>();
    setupRoutes();
}

HttpServer::~HttpServer() {
    if (running_.load()) {
        stop();
    }
}

Result<void> HttpServer::start(int port) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    if (running_.load()) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0, "Server is already running");
    }

    if (port <= 0 || port > 65535) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0, "Invalid port number: " + std::to_string(port));
    }

    // Check if root directory exists, create if not
    std::error_code ec;
    bool exists = std::filesystem::exists(root_directory_, ec);
    if (ec) {
        return Result<void>::make_error(ErrorType::kUnknownError, 0, "Failed to check root directory: " + ec.message());
    }

    if (!exists) {
        // Try to create the directory
        std::filesystem::create_directories(root_directory_, ec);
        if (ec) {
            return Result<void>::make_error(ErrorType::kUnknownError, 0, "Failed to create root directory: " + ec.message());
        }
    } else {
        // Verify it's a directory
        bool is_dir = std::filesystem::is_directory(root_directory_, ec);
        if (ec) {
            return Result<void>::make_error(ErrorType::kUnknownError, 0, "Failed to check if path is directory: " + ec.message());
        }

        if (!is_dir) {
            return Result<void>::make_error(ErrorType::kUnknownError, 0, "Root path is not a directory: " + root_directory_);
        }
    }

    // Start server in a separate thread
    running_.store(true);
    server_->set_base_dir(root_directory_);  // Update base_dir after directory is ready
    server_thread_ = std::jthread(&HttpServer::serverThreadFunc, this, port);

    // Give server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return Result<void>::make_ok();
}

void HttpServer::stop() {
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

bool HttpServer::isRunning() const noexcept {
    return running_.load();
}

void HttpServer::setRootDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    root_directory_ = path;
}

void HttpServer::setupRoutes() {
    // Set static file directory
    // Note: set_base_dir will be called during start() after directory is verified/created
    server_->set_base_dir(root_directory_);

    // Add CORS headers to all responses
    server_->set_pre_routing_handler([](const httplib::Request& /*req*/,
                                         httplib::Response& res) {
        addCorsHeaders(res);
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Handle OPTIONS requests for CORS preflight
    server_->Options(".*", [](const httplib::Request& /*req*/, httplib::Response& res) {
        addCorsHeaders(res);
        res.status = 200;
        return;
    });

    // Add health check endpoint
    server_->Get("/health", [](const httplib::Request& /*req*/, httplib::Response& res) {
        res.set_content(R"({"status":"ok","server":"HttpServer"})", "application/json");
    });

    // Add status endpoint
    server_->Get("/status", [](const httplib::Request& /*req*/, httplib::Response& res) {
        res.set_content(R"({"status":"running","service":"http-server"})", "application/json");
    });
}

void HttpServer::addCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

void HttpServer::serverThreadFunc(int port) {
    try {
        server_->listen("0.0.0.0", port);
    } catch (const std::exception& e) {
        std::cerr << "[HttpServer] Error: " << e.what() << std::endl;
        running_.store(false);
    }
}

} // namespace screensdk::server
