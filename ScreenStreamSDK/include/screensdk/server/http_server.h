/**
 * @file http_server.h
 * @brief HTTP static file server implementation using cpp-httplib
 *
 * This class provides a simple HTTP server for serving static files
 * from a specified root directory. It's designed for internal use
 * within RemoteDesktopServer and does not need to be exported as part
 * of the public SDK API.
 */

#pragma once

// Define before including any Windows headers to avoid conflicts
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <mutex>

#include "screensdk/utils/error.h"

// Forward declaration to avoid including httplib.h in header
namespace httplib {
class Server;
struct Response;
}

namespace screensdk::server {

/**
 * @brief HTTP static file server
 *
 * Provides a simple HTTP server for serving static web files (HTML, CSS, JS, etc.)
 * with CORS support for cross-origin requests from mobile browsers.
 *
 * Usage:
 *   auto server = std::make_unique<HttpServer>();
 *   server->setRootDirectory("web");
 *   server->start(8080);
 *   // ... server is running ...
 *   server->stop();
 */
class HttpServer {
public:
    /**
     * @brief Constructor
     */
    HttpServer();

    /**
     * @brief Destructor - automatically stops server if running
     */
    ~HttpServer();

    // Disable copy and move
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) = delete;
    HttpServer& operator=(HttpServer&&) = delete;

    /**
     * @brief Start the HTTP server on specified port
     *
     * @param port Port number to listen on
     * @return Result<void> Success if server started successfully
     */
    Result<void> start(int port);

    /**
     * @brief Stop the HTTP server
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
     * @brief Set the root directory for serving static files
     *
     * All HTTP requests will be served relative to this directory.
     * For example, if root is "web/", request to "/index.html"
     * will serve "web/index.html".
     *
     * @param path Root directory path (relative or absolute)
     */
    void setRootDirectory(const std::string& path);

private:
    /**
     * @brief Set up HTTP routes and handlers
     */
    void setupRoutes();

    /**
     * @brief Add CORS headers to response
     */
    static void addCorsHeaders(httplib::Response& res);

    /**
     * @brief Server thread function
     */
    void serverThreadFunc(int port);

private:
    std::unique_ptr<httplib::Server> server_;
    std::atomic<bool> running_{false};
    std::string root_directory_{"web"};
    std::jthread server_thread_;
    std::mutex server_mutex_;
};

} // namespace screensdk::server
