/**
 * @file standalone_server_main.cpp
 * @brief Standalone remote desktop server main program
 *
 * This is a standalone server that runs RemoteDesktopServer indefinitely,
 * unlike the integration test which only runs for a short duration.
 *
 * Usage:
 *   standalone_server.exe [options]
 *
 * Options:
 *   --http-port <port>     HTTP server port (default: 18080)
 *   --signaling-port <port> Signaling server port (default: 18081)
 *   --web-root <path>      Web root directory (default: web)
 *   --display-id <id>      Display ID to capture (default: 0)
 *   --fps <fps>            Target frame rate (default: 30)
 *   --help                 Show help message
 */

// Define before including any Windows headers to avoid conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <screensdk/server/remote_desktop_server.h>
#include <screensdk/utils/error.h>
#include <iostream>
#include <csignal>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>

using namespace screensdk;

// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

/**
 * @brief Signal handler for Ctrl+C
 */
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Server] Received shutdown signal, stopping..." << std::endl;
        g_running.store(false);
    }
}

/**
 * @brief Print usage information
 */
void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --http-port <port>     HTTP server port (default: 18080)" << std::endl;
    std::cout << "  --signaling-port <port> Signaling server port (default: 18081)" << std::endl;
    std::cout << "  --web-root <path>      Web root directory (default: web)" << std::endl;
    std::cout << "  --display-id <id>      Display ID to capture (default: 0)" << std::endl;
    std::cout << "  --fps <fps>            Target frame rate (default: 30)" << std::endl;
    std::cout << "  --max-bitrate <bps>     Maximum bitrate in bps (default: 15000000)" << std::endl;
    std::cout << "  --help                 Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " --http-port 8080 --signaling-port 8081" << std::endl;
}

/**
 * @brief Parse command line arguments
 */
ServerConfig parseArguments(int argc, char* argv[]) {
    ServerConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "--http-port" && i + 1 < argc) {
            config.http_port = std::stoi(argv[++i]);
        } else if (arg == "--signaling-port" && i + 1 < argc) {
            config.signaling_port = std::stoi(argv[++i]);
        } else if (arg == "--web-root" && i + 1 < argc) {
            config.web_root = argv[++i];
        } else if (arg == "--display-id" && i + 1 < argc) {
            config.display_id = std::stoi(argv[++i]);
        } else if (arg == "--fps" && i + 1 < argc) {
            config.fps = std::stoi(argv[++i]);
        } else if (arg == "--max-bitrate" && i + 1 < argc) {
            config.max_bitrate_bps = std::stoi(argv[++i]);
        } else {
            std::cerr << "[Server] Unknown option: " << arg << std::endl;
            std::cerr << "[Server] Use --help for usage information" << std::endl;
            exit(1);
        }
    }

    return config;
}

int main(int argc, char* argv[]) {
    std::cout << "============================================" << std::endl;
    std::cout << "  Remote Desktop Server - Standalone" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    // Parse command line arguments
    ServerConfig config = parseArguments(argc, argv);

    // Display configuration
    std::cout << "Configuration:" << std::endl;
    std::cout << "  HTTP Port:      " << config.http_port << std::endl;
    std::cout << "  Signaling Port:  " << config.signaling_port << std::endl;
    std::cout << "  Web Root:       " << config.web_root << std::endl;
    std::cout << "  Display ID:     " << config.display_id << std::endl;
    std::cout << "  FPS:            " << config.fps << std::endl;
    std::cout << "  Max Bitrate:    " << config.max_bitrate_bps << " bps" << std::endl;
    std::cout << std::endl;

    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Create server
    auto server = std::make_unique<RemoteDesktopServer>();

    // Initialize
    std::cout << "[Server] Initializing..." << std::endl;
    [[maybe_unused]] auto init_result = server->initialize(config);
    if (!init_result) {
        std::cerr << "[Server] Initialization failed: " << init_result.error().message << std::endl;
        return 1;
    }
    std::cout << "[Server] Initialization successful" << std::endl;
    std::cout << std::endl;

    // Start
    std::cout << "[Server] Starting..." << std::endl;
    [[maybe_unused]] auto start_result = server->start();
    if (!start_result) {
        std::cerr << "[Server] Start failed: " << start_result.error().message << std::endl;
        return 1;
    }
    std::cout << "[Server] Started successfully" << std::endl;
    std::cout << std::endl;

    // Print access information
    std::cout << "============================================" << std::endl;
    std::cout << "  Server is running!" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Access URLs:" << std::endl;
    std::cout << "  Web Interface:  http://localhost:" << config.http_port << "/index.html" << std::endl;
    std::cout << "  Health Check:  http://localhost:" << config.http_port << "/health" << std::endl;
    std::cout << "  Signaling:     ws://localhost:" << config.signaling_port << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    // Main loop
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop
    std::cout << std::endl;
    std::cout << "[Server] Stopping..." << std::endl;
    server->stop();
    std::cout << "[Server] Stopped" << std::endl;

    // Shutdown
    std::cout << "[Server] Shutting down..." << std::endl;
    server->shutdown();
    std::cout << "[Server] Shutdown complete" << std::endl;
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  Server Stopped" << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}
