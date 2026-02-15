/**
 * @file test_httplib.cpp
 * @brief Test program to verify cpp-httplib integration
 *
 * This is a minimal test to ensure cpp-httplib can be compiled and used.
 * It creates a simple HTTP server and tests basic functionality.
 */

#include <httplib.h>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
  std::cout << "=== cpp-httplib Integration Test ===" << std::endl;
  std::cout << "Version: " << CPPHTTPLIB_VERSION << std::endl;
  std::cout << "Version Num: " << CPPHTTPLIB_VERSION_NUM << std::endl;

  // Create HTTP server on port 18080
  httplib::Server svr;
  int port = 18080;

  // Handle root path
  svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
    res.set_content("<html><body><h1>cpp-httplib Test Server</h1>"
                    "<p>Server is working correctly!</p></body></html>",
                    "text/html");
  });

  // Handle /status endpoint
  svr.Get("/status", [](const httplib::Request& req, httplib::Response& res) {
    res.set_content(R"({"status": "ok", "message": "cpp-httplib is working"})",
                    "application/json");
  });

  // Handle static file serving (test CORS headers)
  svr.Get("/test-cors", [](const httplib::Request& req, httplib::Response& res) {
    // Set CORS headers
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");

    res.set_content(R"({"status": "ok", "cors": "enabled"})",
                    "application/json");
  });

  // Start server
  std::cout << "\nStarting HTTP server on port " << port << "..." << std::endl;

  if (!svr.listen("0.0.0.0", port)) {
    std::cerr << "\n[FAILED] Failed to start server on port " << port << std::endl;
    std::cerr << "The port might be in use. Please check:" << std::endl;
    std::cerr << "  1. No other process is using port " << port << std::endl;
    std::cerr << "  2. Windows Firewall is not blocking the port" << std::endl;
    return 1;
  }

  // This line should never be reached
  return 0;
}
