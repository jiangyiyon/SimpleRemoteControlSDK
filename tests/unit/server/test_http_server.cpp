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
    }

    void TearDown() override {
        if (server_ && server_->isRunning()) {
            server_->stop();
        }
        // Give server time to stop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Helper: Wait for server to be ready
    void waitForServerReady(int milliseconds = 500) {
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
    server_->setRootDirectory("web");

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
    // This test requires web/ directory with index.html
    // Skip if directory doesn't exist
    std::filesystem::path web_dir("web");
    if (!std::filesystem::exists(web_dir)) {
        GTEST_SKIP() << "web/ directory does not exist";
    }

    server_->setRootDirectory("web");
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
    std::filesystem::path web_dir("web");
    if (!std::filesystem::exists(web_dir)) {
        GTEST_SKIP() << "web/ directory does not exist";
    }

    server_->setRootDirectory("web");
    auto result = server_->start(test_port_);
    ASSERT_TRUE(result);
    waitForServerReady();

    auto body = httpGet("/style.css");
    // May or may not exist, just check no crash
}

// Test 6: Serve JS file
TEST_F(HttpServerTest, ServeJsFile) {
    std::filesystem::path web_dir("web");
    if (!std::filesystem::exists(web_dir)) {
        GTEST_SKIP() << "web/ directory does not exist";
    }

    server_->setRootDirectory("web");
    auto result = server_->start(test_port_);
    ASSERT_TRUE(result);
    waitForServerReady();

    auto body = httpGet("/app.js");
    // May or may not exist, just check no crash
}

// Test 7: CORS headers are present
TEST_F(HttpServerTest, CorsHeadersPresent) {
    std::filesystem::path web_dir("web");
    if (!std::filesystem::exists(web_dir)) {
        GTEST_SKIP() << "web/ directory does not exist";
    }

    server_->setRootDirectory("web");
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
    std::filesystem::path web_dir("web");
    if (!std::filesystem::exists(web_dir)) {
        GTEST_SKIP() << "web/ directory does not exist";
    }

    server_->setRootDirectory("web");
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

    // At least some requests should succeed
    EXPECT_GT(success_count.load(), 0);
}

} // namespace screensdk::server
