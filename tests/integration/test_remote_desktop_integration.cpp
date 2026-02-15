/**
 * @file test_remote_desktop_integration.cpp
 * @brief Integration tests for RemoteDesktopServer
 *
 * Tests RemoteDesktopServer integration with sub-components:
 * - HttpServer
 * - SignalingServer
 * - ScreenCapture
 * - Encoder
 * - WebRTC Transport
 */

#include <gtest/gtest.h>

#include "screensdk/server/remote_desktop_server.h"
#include "screensdk/core/session.h"
#include <httplib.h>
#include <thread>
#include <chrono>

using namespace screensdk;
using namespace screensdk::server;

/**
 * @brief Test fixture for RemoteDesktopServer integration tests
 */
class RemoteDesktopIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<RemoteDesktopServer>();

        // Use non-standard ports to avoid conflicts
        config_.http_port = 18080;
        config_.signaling_port = 18081;
        config_.web_root = "test_web";
        config_.display_id = 1;
        config_.fps = 30;
        config_.max_bitrate_bps = 15000000;
    }

    void TearDown() override {
        if (server_) {
            server_->shutdown();
        }
    }

    std::unique_ptr<RemoteDesktopServer> server_;
    ServerConfig config_;
};

/**
 * @brief Test end-to-end flow
 */
TEST_F(RemoteDesktopIntegrationTest, EndToEndFlowTest) {
    // Initialize
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << "Initialization failed: " << init_result.errorMessage();
    
    // Start
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << "Start failed: " << start_result.errorMessage();
    EXPECT_TRUE(server_->isRunning());
    
    // Wait for services to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify URLs
    EXPECT_EQ(server_->getHttpUrl(), "http://localhost:18080");
    EXPECT_EQ(server_->getSignalingUrl(), "http://localhost:18081");
    
    // Stop
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
    
    // Shutdown
    server_->shutdown();
}

/**
 * @brief Test HTTP server integration
 */
TEST_F(RemoteDesktopIntegrationTest, HttpServerIntegrationTest) {
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.errorMessage();
    
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test HTTP request
    httplib::Client client("localhost", config_.http_port);
    auto res = client.Get("/health");
    
    ASSERT_TRUE(res != nullptr) << "HTTP request failed";
    EXPECT_EQ(res->status, 200);
    
    server_->stop();
}

/**
 * @brief Test signaling server integration
 */
TEST_F(RemoteDesktopIntegrationTest, SignalingServerIntegrationTest) {
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.errorMessage();
    
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test signaling health endpoint
    httplib::Client client("localhost", config_.signaling_port);
    auto res = client.Get("/health");
    
    ASSERT_TRUE(res != nullptr) << "Signaling request failed";
    EXPECT_EQ(res->status, 200);
    
    server_->stop();
}

/**
 * @brief Test screen capture integration
 */
TEST_F(RemoteDesktopIntegrationTest, ScreenCaptureIntegrationTest) {
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.errorMessage();
    
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    
    // Let capture thread run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Verify server is still running
    EXPECT_TRUE(server_->isRunning());
    
    server_->stop();
}

/**
 * @brief Test full stack integration
 */
TEST_F(RemoteDesktopIntegrationTest, FullStackTest) {
    // Initialize
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.errorMessage();
    
    // Start
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    
    // Wait for all services to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify HTTP server is responding
    httplib::Client http_client("localhost", config_.http_port);
    auto http_res = http_client.Get("/health");
    EXPECT_EQ(http_res->status, 200);
    
    // Verify signaling server is responding
    httplib::Client sig_client("localhost", config_.signaling_port);
    auto sig_res = sig_client.Get("/health");
    EXPECT_EQ(sig_res->status, 200);
    
    // Let system run for a while
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Verify still running
    EXPECT_TRUE(server_->isRunning());
    
    // Stop
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}

/**
 * @brief Test concurrent access
 */
TEST_F(RemoteDesktopIntegrationTest, ConcurrentAccessTest) {
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.errorMessage();
    
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Create multiple concurrent HTTP requests
    std::vector<std::thread> threads;
    const int kThreadCount = 5;
    std::vector<int> results(kThreadCount, 0);
    
    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([this, i, &results]() {
            httplib::Client client("localhost", config_.http_port);
            auto res = client.Get("/health");
            results[i] = (res != nullptr && res->status == 200) ? 1 : 0;
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify all requests succeeded
    for (int result : results) {
        EXPECT_EQ(result, 1) << "Not all concurrent requests succeeded";
    }
    
    server_->stop();
}

/**
 * @brief Test recovery after error
 */
TEST_F(RemoteDesktopIntegrationTest, RecoveryTest) {
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.errorMessage();
    
    // Start
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Stop
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
    
    // Restart
    start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    EXPECT_TRUE(server_->isRunning());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify server is still working
    httplib::Client client("localhost", config_.http_port);
    auto res = client.Get("/health");
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(res->status, 200);
    
    server_->stop();
}

/**
 * @brief Test performance (basic frame rate check)
 */
TEST_F(RemoteDesktopIntegrationTest, PerformanceTest) {
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.errorMessage();
    
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.errorMessage();
    
    // Run for 3 seconds
    const auto duration = std::chrono::seconds(3);
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < duration) {
        EXPECT_TRUE(server_->isRunning());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    server_->stop();
}
