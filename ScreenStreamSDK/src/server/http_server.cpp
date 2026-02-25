/**
 * @file http_server.cpp
 * @brief HTTP static file server implementation
 */

// Define before including any Windows headers to avoid conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "screensdk/server/http_server.h"
#include "screensdk/utils/error.h"
#include <httplib.h>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <mutex>

// We need the full Response definition here
namespace httplib {
using Response = struct Response;
}

namespace screensdk {

/**
 * @brief HTTP server implementation
 *
 * Internal implementation class for IHttpServer interface.
 * Uses cpp-httplib to provide static file serving with CORS support.
 */
class HttpServerImpl : public IHttpServer {
public:
  HttpServerImpl();
  ~HttpServerImpl() override;

  // Disable copy and move
  HttpServerImpl(const HttpServerImpl&) = delete;
  HttpServerImpl& operator=(const HttpServerImpl&) = delete;
  HttpServerImpl(HttpServerImpl&&) = delete;
  HttpServerImpl& operator=(HttpServerImpl&&) = delete;

  Result<void> start(int port) override;
  void stop() override;
  bool isRunning() const noexcept override;
  Result<void> setRootDirectory(const std::string& path) override;
  std::string getRootDirectory() const override;
  void onWebSocketConnection(WebSocketCallback callback) override;

private:
  void setupRoutes();
  static void addCorsHeaders(httplib::Response& res);
  void serverThreadFunc(int port);

private:
  std::unique_ptr<httplib::Server> server_;
  std::atomic<bool> running_{false};
  std::string root_directory_{"web"};
  std::thread server_thread_;
  mutable std::recursive_mutex server_mutex_;
  WebSocketCallback ws_callback_;
};

HttpServerImpl::HttpServerImpl() {
  server_ = std::make_unique<httplib::Server>();
  // Note: set_base_dir will be called in start() after verifying the directory
}

HttpServerImpl::~HttpServerImpl() {
  if (running_.load()) {
    stop();
  }
}

Result<void> HttpServerImpl::start(int port) {
  std::cout << "[HttpServer] start() called with port: " << port << std::endl;
  std::lock_guard<std::recursive_mutex> lock(server_mutex_);

  std::cout << "[HttpServer] Checking running state..." << std::endl;
  if (running_.load()) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0, "Server is already running");
  }

  std::cout << "[HttpServer] Checking port validity..." << std::endl;
  if (port <= 0 || port > 65535) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0, "Invalid port number: " + std::to_string(port));
  }

  // Check if root directory exists, create if not
  std::cout << "[HttpServer] Checking root directory: " << root_directory_ << std::endl;
  std::error_code ec;
  bool exists = std::filesystem::exists(root_directory_, ec);
  if (ec) {
    std::cerr << "[HttpServer] Failed to check root directory: " << ec.message() << std::endl;
    return Result<void>::make_error(ErrorType::kUnknownError, 0, "Failed to check root directory: " + ec.message());
  }

  std::cout << "[HttpServer] Root directory exists: " << (exists ? "yes" : "no") << std::endl;
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

  // Set base directory for static file serving
  std::cout << "[HttpServer] Setting base directory: " << root_directory_ << std::endl;
  try {
    bool set_dir_result = server_->set_base_dir(root_directory_);
    if (!set_dir_result) {
      std::cerr << "[HttpServer] Failed to set base directory" << std::endl;
      return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                       "Failed to set base directory: " + root_directory_);
    }
    std::cout << "[HttpServer] Base directory set successfully" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "[HttpServer] Exception in set_base_dir: " << e.what() << std::endl;
    return Result<void>::make_error(ErrorType::kUnknownError, 0,
                                     "Exception in set_base_dir: " + std::string(e.what()));
  }

  // Setup routes
  try {
    setupRoutes();
  } catch (const std::exception& e) {
    std::cerr << "[HttpServer] Exception in setupRoutes: " << e.what() << std::endl;
  }

  std::cout << "[HttpServer] Starting server thread..." << std::endl;
  std::cout.flush();
  // Use std::thread instead of std::jthread
  server_thread_ = std::thread(&HttpServerImpl::serverThreadFunc, this, port);
  std::cout << "[HttpServer] Server thread started" << std::endl;
  std::cout.flush();

  // Give server a moment to start
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return Result<void>::make_ok();
}

void HttpServerImpl::stop() {
  std::lock_guard<std::recursive_mutex> lock(server_mutex_);

  if (!running_.load()) {
    return;
  }

  running_.store(false);

  // Stop the server
  if (server_) {
    server_->stop();
  }

  // Join the thread
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
}

bool HttpServerImpl::isRunning() const noexcept {
  return running_.load();
}

Result<void> HttpServerImpl::setRootDirectory(const std::string& path) {
  std::lock_guard<std::recursive_mutex> lock(server_mutex_);

  if (running_.load()) {
    return Result<void>::make_error(ErrorType::kUnknownError, 0, "Cannot set root directory while server is running");
  }

  root_directory_ = path;
  return Result<void>::make_ok();
}

std::string HttpServerImpl::getRootDirectory() const {
  std::lock_guard<std::recursive_mutex> lock(server_mutex_);
  return root_directory_;
}

void HttpServerImpl::onWebSocketConnection(WebSocketCallback callback) {
  std::lock_guard<std::recursive_mutex> lock(server_mutex_);
  ws_callback_ = std::move(callback);
}

void HttpServerImpl::setupRoutes() {
  // Note: base directory is set in start() after verification

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

void HttpServerImpl::addCorsHeaders(httplib::Response& res) {
  res.set_header("Access-Control-Allow-Origin", "*");
  res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

void HttpServerImpl::serverThreadFunc(int port) {
  try {
    server_->listen("0.0.0.0", port);
  } catch (const std::exception& e) {
    std::cerr << "[HttpServer] Error: " << e.what() << std::endl;
    running_.store(false);
  }
}

} // namespace screensdk

// Factory functions (outside namespace for C linkage)
extern "C" screensdk::IHttpServer* CreateHttpServer() {
  return new screensdk::HttpServerImpl();
}

extern "C" void DestroyHttpServer(screensdk::IHttpServer* server) {
  if (server != nullptr) {
    delete server;
  }
}
