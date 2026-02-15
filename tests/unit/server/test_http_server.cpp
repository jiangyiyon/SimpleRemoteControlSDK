/**
 * @file test_http_server.cpp
 * @brief Unit tests for HttpServer class
 *
 * Test cases:
 * 1. Start and stop server
 * 2. Set root directory
 * 3. Serve static files (index.html, CSS, JS)
 * 4. CORS headers
 * 5. Concurrent requests
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <httplib.h>

#include "../../ScreenStreamSDK/src/server/http_server.h"

namespace screensdk::server {

class HttpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<HttpServer>();
        test_port_ = 18090; // Use a different port to avoid conflicts

        // Set test web directory relative to binary location
        test_web_dir_ = "../../test_web";
        std::error_code ec;
        bool exists = std::filesystem::exists(test_web_dir_, ec);
        if (!exists || ec) {
            // Fallback to absolute path if relative doesn't work
            test_web_dir_ = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "test_web";
        }
    }

    void TearDown() override {
        if (server_ && server_->isRunning()) {
            server_->stop();
        }
        // Give server time to stop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Helper: Wait for server to be ready
    void waitForServerReady(int milliseconds = 1000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    // Helper: Make HTTP GET request
    std::optional<std::string> httpGet(const std::string& path) {
        httplib::Client client("localhost", test_port_);
        client.set_connection_timeout(5);
        client.set_read_timeout(5);

        auto res = client.Get(path);
        if (!res) {
            return std::nullopt;
        }
        if (res->status != 200) {
            return std::nullopt;
        }
        return res->body;
    }

    // Helper: Check CORS headers
    bool hasCorsHeaders(const std::string& path) {
        httplib::Client client("localhost", test_port_);
        client.set_connection_timeout(5);
        client.set_read_timeout(5);

        auto res = client.Get(path);
        if (!res) {
            return false;
        }
        auto it = res->headers.find("Access-Control-Allow-Origin");
        return it != res->headers.end() && it->second == "*";
    }

    std::unique_ptr<HttpServer> server_;
    int test_port_;
    std::filesystem::path test_web_dir_;
};

// Test 1: Start and stop server
TEST_F(HttpServerTest, StartAndStopServer) {
    // Server should not be running initially
    EXPECT_FALSE(server_->isRunning());

    // Start server
    auto result = server_->start(test_port_);
    EXPECT_TRUE(result);
    EXPECT_TRUE(server_->isRunning());

    // Stop server
    server_->stop();
    waitForServerReady(100);
    EXPECT_FALSE(server_->isRunning());
}

// Test 2: Start server on out-of-range port
TEST_F(HttpServerTest, StartOnInvalidPort) {
    // Try to start on port 70000 (out of valid range)
    auto result = server_->start(70000);
    EXPECT_FALSE(result);
}

// Test 3: Set root directory
TEST_F(HttpServerTest, SetRootDirectory) {
    server_->setRootDirectory(test_web_dir_.string());

    // Start server
    auto result = server_->start(test_port_);
    EXPECT_TRUE(result);

    // Wait for server to be ready
    waitForServerReady();

    // Note: Actual file serving test requires web/ directory to exist
    // This test verifies the setter works without crashing
}

// Test 4: Serve static file (index.html)
TEST_F(HttpServerTest, ServeStaticFile) {
    // Check if test web directory exists
    if (!std::filesystem::exists(test_web_dir_)) {
        GTEST_SKIP() << "Test web directory does not exist: " << test_web_dir_.string();
    }

    server_->setRootDirectory(test_web_dir_.string());
    auto result = server_->start(test_port_);
    ASSERT_TRUE(result);
    waitForServerReady();

    auto body = httpGet("/index.html");
    ASSERT_TRUE(body.has_value());
    EXPECT_FALSE(body->empty());
    EXPECT_TRUE(body->find("<!DOCTYPE") != std::string::npos ||
                body->find("<html") != std::string::npos);
}

// Test 5: Serve CSS file
TEST_F(HttpServerTest, ServeCssFile) {
    if (!std::filesystem::exists(test_web_dir_)) {
        GTEST_SKIP() << "Test web directory does not exist: " << test_web_dir_.string();
    }

    server_->setRootDirectory(test_web_dir_.string());
    auto result = server_->start(test_port_);
    ASSERT_TRUE(result);
    waitForServerReady();

    auto body = httpGet("/style.css");
    // CSS file should exist in test directory
    ASSERT_TRUE(body.has_value());
    EXPECT_TRUE(body->find("font-family") != std::string::npos);
}

// Test 6: Serve JS file
TEST_F(HttpServerTest, ServeJsFile) {
    if (!std::filesystem::exists(test_web_dir_)) {
        GTEST_SKIP() << "Test web directory does not exist: " << test_web_dir_.string();
    }

    server_->setRootDirectory(test_web_dir_.string());
    auto result = server_->start(test_port_);
    ASSERT_TRUE(result);
    waitForServerReady();

    auto body = httpGet("/app.js");
    // JS file should exist in test directory
    ASSERT_TRUE(body.has_value());
    EXPECT_TRUE(body->find("console.log") != std::string::npos);
}

// Test 7: CORS headers are present
TEST_F(HttpServerTest, CorsHeadersPresent) {
    if (!std::filesystem::exists(test_web_dir_)) {
        GTEST_SKIP() << "Test web directory does not exist: " << test_web_dir_.string();
    }

    server_->setRootDirectory(test_web_dir_.string());
    auto result = server_->start(test_port_);
    ASSERT_TRUE(result);
    waitForServerReady();

    EXPECT_TRUE(hasCorsHeaders("/index.html"));
}

// Test 8: Multiple start calls (should fail if already running)
TEST_F(HttpServerTest, MultipleStartCalls) {
    auto result1 = server_->start(test_port_);
    ASSERT_TRUE(result1);

    // Second start should fail
    auto result2 = server_->start(test_port_);
    EXPECT_FALSE(result2);
}

// Test 9: Stop when not running (should not crash)
TEST_F(HttpServerTest, StopWhenNotRunning) {
    EXPECT_FALSE(server_->isRunning());
    server_->stop(); // Should not crash
    EXPECT_FALSE(server_->isRunning());
}

// Test 10: Concurrent requests
TEST_F(HttpServerTest, ConcurrentRequests) {
    if (!std::filesystem::exists(test_web_dir_)) {
        GTEST_SKIP() << "Test web directory does not exist: " << test_web_dir_.string();
    }

    server_->setRootDirectory(test_web_dir_.string());
    auto result = server_->start(test_port_);
    ASSERT_TRUE(result);
    waitForServerReady();

    // Make multiple concurrent requests
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, &success_count]() {
            auto body = httpGet("/index.html");
            if (body.has_value()) {
                success_count++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All requests should succeed
    EXPECT_EQ(success_count.load(), 5);
}

} // namespace screensdk::server
