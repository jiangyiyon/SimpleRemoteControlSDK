#pragma once

#include "screensdk/export.h"
#include "screensdk/utils/error.h"
#include <string>
#include <functional>

namespace screensdk {

/**
 * @brief HTTP server interface for serving static files
 *
 * Provides a pure virtual interface for maximum ABI compatibility.
 * This interface allows serving static web files (HTML, CSS, JS, etc.)
 * with CORS support for cross-origin requests from mobile browsers.
 *
 * T1001: Implement HTTP static file server
 *
 * Usage:
 *   IHttpServer* server = CreateHttpServer();
 *   server->setRootDirectory("web");
 *   server->start(8080);
 *   // ... server is running ...
 *   server->stop();
 *   DestroyHttpServer(server);
 */
struct SCREEN_STREAM_SDK_EXPORT IHttpServer {
  virtual ~IHttpServer() = default;

  /**
   * @brief Start the HTTP server on specified port
   * @param port Port number to listen on (1-65535)
   * @return Result<void> Success if server started successfully
   */
  virtual Result<void> start(int port) = 0;

  /**
   * @brief Stop the HTTP server
   *
   * This method is thread-safe and can be called from any thread.
   * It will block until the server is fully stopped.
   */
  virtual void stop() = 0;

  /**
   * @brief Check if the server is currently running
   * @return true if server is running, false otherwise
   */
  virtual bool isRunning() const noexcept = 0;

  /**
   * @brief Set the root directory for serving static files
   *
   * All HTTP requests will be served relative to this directory.
   * For example, if root is "web/", request to "/index.html"
   * will serve "web/index.html".
   *
   * @param path Root directory path (relative or absolute)
   * @return Result<void> Success if directory is valid
   */
  virtual Result<void> setRootDirectory(const std::string& path) = 0;

  /**
   * @brief Get the current root directory
   * @return Current root directory path
   */
  virtual std::string getRootDirectory() const = 0;

  /**
   * @brief Callback type for WebSocket connections (for future use)
   */
  using WebSocketCallback = std::function<void()>;

  /**
   * @brief Set callback for new WebSocket connections
   * @param callback Function to call when new WebSocket connection is established
   */
  virtual void onWebSocketConnection(WebSocketCallback callback) = 0;
};

/**
 * @brief Create a new HTTP server instance
 *
 * Creates a new HTTP server instance for serving static files.
 * The server must be destroyed using DestroyHttpServer() to avoid memory leaks.
 *
 * @return HTTP server instance, or nullptr on failure
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IHttpServer* CreateHttpServer();

/**
 * @brief Destroy an HTTP server instance
 * @param server HTTP server instance to destroy (nullptr is safe)
 */
extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyHttpServer(IHttpServer* server);

} // namespace screensdk
