/**
 * @file test_server_main.cpp
 * @brief Simple test server for T1004 - Real Device Testing
 *
 * This is a standalone server executable for testing RemoteDesktopServer
 * on real devices (Android, iOS, Desktop browsers).
 *
 * Usage:
 *   test_server.exe
 *
 * Configuration:
 *   Edit ServerConfig in main() function
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

#include "screensdk/server/remote_desktop_server.h"

using namespace screensdk;

// Global server pointer for signal handler
RemoteDesktopServer* g_server = nullptr;

/**
 * @brief Signal handler for graceful shutdown
 */
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

/**
 * @brief Print server info
 */
void printServerInfo(const RemoteDesktopServer& server, const ServerConfig& config) {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "  RemoteDesktopServer - Test Mode" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "\nServer Information:" << std::endl;
    std::cout << "  HTTP Server:      " << server.getHttpUrl() << std::endl;
    std::cout << "  Signaling Server: " << server.getSignalingUrl() << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  HTTP Port:        " << config.http_port << std::endl;
    std::cout << "  Signaling Port:    " << config.signaling_port << std::endl;
    std::cout << "  Web Root:         " << config.web_root << std::endl;
    std::cout << "  Display ID:       " << config.display_id << std::endl;
    std::cout << "  FPS:              " << config.fps << std::endl;
    std::cout << "  Max Bitrate:     " << config.max_bitrate_bps << " bps" << std::endl;
    std::cout << "  STUN Server:      " << config.stun_server << std::endl;
    std::cout << "\n===========================================" << std::endl;
}

int main() {
    std::cout << "\nRemoteDesktopServer Test Launcher" << std::endl;
    std::cout << "=================================" << std::endl;

    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Create server
    auto server = std::make_unique<RemoteDesktopServer>();
    g_server = server.get();

    // Configure server
    ServerConfig config;
    config.http_port = 8080;
    config.signaling_port = 8081;
    config.web_root = "web";
    config.display_id = 0;
    config.fps = 30;
    config.max_bitrate_bps = 15000000;
    config.stun_server = "stun:stun.l.google.com:19302";

    // Set callbacks
    server->setStateChangeCallback([](SessionState state) {
        const char* stateStr = "Unknown";
        switch (state) {
            case SessionState::kDisconnected:
                stateStr = "Disconnected";
                break;
            case SessionState::kConnecting:
                stateStr = "Connecting";
                break;
            case SessionState::kConnected:
                stateStr = "Connected";
                break;
            case SessionState::kError:
                stateStr = "Error";
                break;
        }
        std::cout << "[State] " << stateStr << std::endl;
    });

    server->setErrorCallback([](const std::string& error) {
        std::cerr << "[Error] " << error << std::endl;
    });

    // Initialize
    std::cout << "Initializing server..." << std::endl;
    auto initResult = server->initialize(config);
    if (!initResult) {
        std::cerr << "Initialization failed: " << initResult.error().message << std::endl;
        return 1;
    }
    std::cout << "Server initialized successfully." << std::endl;

    // Start
    std::cout << "Starting server..." << std::endl;
    auto startResult = server->start();
    if (!startResult) {
        std::cerr << "Start failed: " << startResult.error().message << std::endl;
        return 1;
    }
    std::cout << "Server started successfully." << std::endl;

    // Print server info
    printServerInfo(*server, config);

    // Print test instructions
    std::cout << "\nTest Instructions:" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << "1. Desktop Testing:" << std::endl;
    std::cout << "   Open browser and navigate to:" << std::endl;
    std::cout << "   http://localhost:8080" << std::endl;
    std::cout << "\n2. Mobile Testing:" << std::endl;
    std::cout << "   a. Ensure mobile device is on same network as PC" << std::endl;
    std::cout << "   b. Get PC IP address (run: ipconfig)" << std::endl;
    std::cout << "   c. On mobile, open browser and navigate to:" << std::endl;
    std::cout << "   http://<PC_IP>:8080" << std::endl;
    std::cout << "\n3. To stop server:" << std::endl;
    std::cout << "   Press Ctrl+C" << std::endl;
    std::cout << "\n===========================================\n" << std::endl;

    // Keep server running
    while (server->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\nShutting down server..." << std::endl;
    server->shutdown();
    g_server = nullptr;

    std::cout << "Server shutdown complete." << std::endl;
    std::cout << "\n===========================================\n" << std::endl;

    return 0;
}
