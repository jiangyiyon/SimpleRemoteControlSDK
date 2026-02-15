/**
 * @file test_remote_desktop_server.cpp
 * @brief Unit tests for RemoteDesktopServer
 *
 * Tests RemoteDesktopServer class functionality including:
 * - Initialization
 * - Start/Stop
 * - Configuration
 * - Callbacks
 * - Error handling
 */

#include <gtest/gtest.h>

#include "screensdk/server/remote_desktop_server.h"
#include "screensdk/core/session.h"

using namespace screensdk;
using namespace screensdk::server;

/**
 * @brief Test fixture for RemoteDesktopServer tests
 */
class RemoteDesktopServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<RemoteDesktopServer>();

        // Default configuration
        config_.http_port = 18080;
        config_.signaling_port = 18081;
        config_.web_root = "test_web";
        config_.display_id = 1;
        config_.fps = 30;
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
 * @brief Test constructor initializes server correctly
 */
TEST_F(RemoteDesktopServerTest, ConstructorTest) {
    ASSERT_NE(server_, nullptr);
    EXPECT_FALSE(server_->isRunning());
}

/**
 * @brief Test initialize success
 */
TEST_F(RemoteDesktopServerTest, InitializeSuccessTest) {
    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << "Initialization failed: " << result.error().message;
}

/**
 * @brief Test initialize with invalid port
 */
TEST_F(RemoteDesktopServerTest, InitializeFailureTest) {
    // Invalid port (out of range)
    config_.http_port = -1;
    
    auto result = server_->initialize(config_);
    EXPECT_FALSE(result);
}

/**
 * @brief Test start and stop
 */
TEST_F(RemoteDesktopServerTest, StartAndStopTest) {
    // Initialize
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.error().message;

    // Start
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.error().message;
    EXPECT_TRUE(server_->isRunning());
    
    // Wait a bit for services to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}

/**
 * @brief Test start without initialize should fail
 */
TEST_F(RemoteDesktopServerTest, StartWithoutInitializeTest) {
    auto result = server_->start();
    EXPECT_FALSE(result);
}

/**
 * @brief Test set HTTP port
 */
TEST_F(RemoteDesktopServerTest, SetHttpPortTest) {
    server_->setHttpPort(9090);
    config_.http_port = 9090;

    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    EXPECT_EQ(server_->getHttpUrl(), "http://localhost:9090");
    
    server_->stop();
}

/**
 * @brief Test set signaling port
 */
TEST_F(RemoteDesktopServerTest, SetSignalingPortTest) {
    server_->setSignalingPort(9091);
    config_.signaling_port = 9091;

    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    EXPECT_EQ(server_->getSignalingUrl(), "http://localhost:9091");
    
    server_->stop();
}

/**
 * @brief Test set web root directory
 */
TEST_F(RemoteDesktopServerTest, SetWebRootDirectoryTest) {
    server_->setWebRootDirectory("custom_web");

    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    server_->stop();
}

/**
 * @brief Test get HTTP URL
 */
TEST_F(RemoteDesktopServerTest, GetHttpUrlTest) {
    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    EXPECT_EQ(server_->getHttpUrl(), "http://localhost:18080");
    
    server_->stop();
}

/**
 * @brief Test get signaling URL
 */
TEST_F(RemoteDesktopServerTest, GetSignalingUrlTest) {
    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    EXPECT_EQ(server_->getSignalingUrl(), "http://localhost:18081");
    
    server_->stop();
}

/**
 * @brief Test isRunning state
 */
TEST_F(RemoteDesktopServerTest, IsRunningTest) {
    // Not running initially
    EXPECT_FALSE(server_->isRunning());

    // Initialize
    auto init_result = server_->initialize(config_);
    ASSERT_TRUE(init_result) << init_result.error().message;

    // Still not running after initialize
    EXPECT_FALSE(server_->isRunning());

    // Start
    auto start_result = server_->start();
    ASSERT_TRUE(start_result) << start_result.error().message;
    EXPECT_TRUE(server_->isRunning());
    
    // Stop
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}

/**
 * @brief Test state change callback
 */
TEST_F(RemoteDesktopServerTest, CallbackTest) {
    bool callback_called = false;
    SessionState captured_state = SessionState::kDisconnected;

    server_->setStateChangeCallback([&](SessionState state) {
        callback_called = true;
        captured_state = state;
    });

    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    // Wait a bit for potential state changes
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server_->stop();
}

/**
 * @brief Test error callback
 */
TEST_F(RemoteDesktopServerTest, ErrorCallbackTest) {
    bool callback_called = false;
    std::string captured_error;

    server_->setErrorCallback([&](const std::string& error) {
        callback_called = true;
        captured_error = error;
    });

    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server_->stop();
}

/**
 * @brief Test shutdown cleans up resources
 */
TEST_F(RemoteDesktopServerTest, ShutdownTest) {
    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    
    EXPECT_TRUE(server_->isRunning());
    
    // Shutdown
    server_->shutdown();
    EXPECT_FALSE(server_->isRunning());
    
    // Multiple shutdown calls should be safe
    server_->shutdown();
}

/**
 * @brief Test multiple start/stop cycles
 */
TEST_F(RemoteDesktopServerTest, MultipleStartStopTest) {
    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;

    // First cycle
    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    EXPECT_TRUE(server_->isRunning());
    
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
    
    // Second cycle
    result = server_->start();
    ASSERT_TRUE(result) << result.error().message;
    EXPECT_TRUE(server_->isRunning());
    
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}

/**
 * @brief Test stop without start should be safe
 */
TEST_F(RemoteDesktopServerTest, StopWithoutStartTest) {
    auto result = server_->initialize(config_);
    ASSERT_TRUE(result) << result.error().message;
    
    // Stop without start should not crash
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}
