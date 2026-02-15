// Define before including any Windows headers to avoid conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

#include "screensdk/server/http_server.h"
#include "httplib.h"

namespace screensdk {

class HttpServerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create HttpServer instance using factory function
    server_ = CreateHttpServer();
    ASSERT_NE(server_, nullptr);

    // Set up test web directory
    test_web_dir_ = "test_web_" + std::to_string(std::random_device{}());

    // Create test directory and files
    std::error_code ec;
    std::filesystem::create_directories(test_web_dir_, ec);
    ASSERT_FALSE(ec) << "Failed to create test directory: " << ec.message();

    // Create test HTML file
    std::ofstream(test_web_dir_ + "/index.html")
        << "<!DOCTYPE html><html><body>Test Page</body></html>";

    // Create test CSS file
    std::ofstream(test_web_dir_ + "/test.css")
        << "body { color: red; }";

    // Create test JS file
    std::ofstream(test_web_dir_ + "/test.js")
        << "console.log('test');";

    // Create test JSON file
    std::ofstream(test_web_dir_ + "/test.json")
        << R"({"status":"ok","message":"test"})";
  }

  void TearDown() override {
    if (server_) {
      if (server_->isRunning()) {
        server_->stop();
      }
      DestroyHttpServer(server_);
      server_ = nullptr;
    }

    // Clean up test directory
    std::error_code ec;
    std::filesystem::remove_all(test_web_dir_, ec);
    // Ignore cleanup errors
  }

  // Helper function to wait for server to start
  void waitForServerRunning() {
    int attempts = 0;
    while (!server_->isRunning() && attempts < 20) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      attempts++;
    }
    ASSERT_TRUE(server_->isRunning()) << "Server did not start within 1 second";
  }

  // Helper function to wait for server to stop
  void waitForServerStopped() {
    int attempts = 0;
    while (server_->isRunning() && attempts < 20) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      attempts++;
    }
    ASSERT_FALSE(server_->isRunning()) << "Server did not stop within 1 second";
  }

  IHttpServer* server_;
  std::string test_web_dir_;
};

// T1001: Create and destroy HTTP server
TEST_F(HttpServerTest, CreateAndDestroyHttpServer) {
  // Server created in SetUp()
  ASSERT_NE(server_, nullptr);

  // Destroy in TearDown() will be called
}

// T1001: Start and stop server
TEST_F(HttpServerTest, StartAndStopServer) {
  ASSERT_FALSE(server_->isRunning());

  auto result = server_->start(18090);
  ASSERT_TRUE(result) << "Failed to start server: " << result.error().message;

  waitForServerRunning();
  EXPECT_TRUE(server_->isRunning());

  server_->stop();
  waitForServerStopped();
  EXPECT_FALSE(server_->isRunning());
}

// T1001: Set root directory before start
TEST_F(HttpServerTest, SetRootDirectoryBeforeStart) {
  auto result = server_->setRootDirectory(test_web_dir_);
  ASSERT_TRUE(result) << "Failed to set root directory: " << result.error().message;

  EXPECT_EQ(server_->getRootDirectory(), test_web_dir_);

  // Start server with the root directory
  result = server_->start(18091);
  ASSERT_TRUE(result);

  waitForServerRunning();

  // Verify server is running
  EXPECT_TRUE(server_->isRunning());

  server_->stop();
  waitForServerStopped();
}

// T1001: Serve index.html
TEST_F(HttpServerTest, ServeIndexHtml) {
  server_->setRootDirectory(test_web_dir_);
  auto result = server_->start(18092);
  ASSERT_TRUE(result);

  waitForServerRunning();

  // Make HTTP request to get index.html
  httplib::Client client("localhost", 18092);
  auto res = client.Get("/index.html");

  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res->status, 200);

  // Debug: print response body if test fails
  if (res->body.find("Test Page") == std::string::npos) {
    std::cout << "DEBUG: Response body: " << res->body << std::endl;
    std::cout << "DEBUG: Content-Type: " << (res->has_header("Content-Type") ? res->get_header_value("Content-Type") : "N/A") << std::endl;
  }

  // Check that the response contains the expected content
  EXPECT_NE(res->body.find("Test Page"), std::string::npos);

  server_->stop();
  waitForServerStopped();
}

// T1001: Serve CSS files
TEST_F(HttpServerTest, ServeCssFiles) {
  server_->setRootDirectory(test_web_dir_);
  auto result = server_->start(18093);
  ASSERT_TRUE(result);

  waitForServerRunning();

  httplib::Client client("localhost", 18093);
  auto res = client.Get("/test.css");

  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, "body { color: red; }");
  EXPECT_EQ(res->get_header_value("Content-Type"), "text/css");

  server_->stop();
  waitForServerStopped();
}

// T1001: Serve JS files
TEST_F(HttpServerTest, ServeJsFiles) {
  server_->setRootDirectory(test_web_dir_);
  auto result = server_->start(18094);
  ASSERT_TRUE(result);

  waitForServerRunning();

  httplib::Client client("localhost", 18094);
  auto res = client.Get("/test.js");

  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, "console.log('test');");
  EXPECT_EQ(res->get_header_value("Content-Type"), "text/javascript");

  server_->stop();
  waitForServerStopped();
}

// T1001: Serve JSON files
TEST_F(HttpServerTest, ServeJsonFiles) {
  server_->setRootDirectory(test_web_dir_);
  auto result = server_->start(18095);
  ASSERT_TRUE(result);

  waitForServerRunning();

  httplib::Client client("localhost", 18095);
  auto res = client.Get("/test.json");

  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, R"({"status":"ok","message":"test"})");
  EXPECT_EQ(res->get_header_value("Content-Type"), "application/json");

  server_->stop();
  waitForServerStopped();
}

// T1001: Add CORS headers
TEST_F(HttpServerTest, AddCorsHeaders) {
  server_->setRootDirectory(test_web_dir_);
  auto result = server_->start(18096);
  ASSERT_TRUE(result);

  waitForServerRunning();

  httplib::Client client("localhost", 18096);
  auto res = client.Get("/index.html");

  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "*");
  EXPECT_EQ(res->get_header_value("Access-Control-Allow-Methods"), "GET, POST, OPTIONS");
  EXPECT_EQ(res->get_header_value("Access-Control-Allow-Headers"), "Content-Type, Authorization");

  server_->stop();
  waitForServerStopped();
}

// T1001: Handle nonexistent file
TEST_F(HttpServerTest, HandleNonexistentFile) {
  server_->setRootDirectory(test_web_dir_);
  auto result = server_->start(18097);
  ASSERT_TRUE(result);

  waitForServerRunning();

  httplib::Client client("localhost", 18097);
  auto res = client.Get("/nonexistent.html");

  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res->status, 404);

  server_->stop();
  waitForServerStopped();
}

// T1001: Concurrent requests
TEST_F(HttpServerTest, ConcurrentRequests) {
  server_->setRootDirectory(test_web_dir_);
  auto result = server_->start(18098);
  ASSERT_TRUE(result);

  waitForServerRunning();

  // Make multiple concurrent requests
  std::vector<std::thread> threads;
  std::vector<int> statuses(10);

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([this, i, &statuses]() {
      httplib::Client client("localhost", 18098);
      auto res = client.Get("/index.html");
      statuses[i] = res ? res->status : -1;
    });
  }

  // Wait for all threads to complete
  for (auto& t : threads) {
    t.join();
  }

  // Verify all requests succeeded
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(statuses[i], 200) << "Request " << i << " failed with status " << statuses[i];
  }

  server_->stop();
  waitForServerStopped();
}

// T1001: Server port binding
TEST_F(HttpServerTest, ServerPortBinding) {
  // Start server on port 18099
  auto result = server_->start(18099);
  ASSERT_TRUE(result);

  waitForServerRunning();

  // Give server more time to fully start (httplib listen() needs time to bind)
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Verify server is accessible
  httplib::Client client("localhost", 18099);
  client.set_connection_timeout(3, 0);
  client.set_read_timeout(3, 0);

  auto res = client.Get("/health");
  ASSERT_NE(res, nullptr) << "Failed to connect to server - server may not be listening yet";
  EXPECT_EQ(res->status, 200);

  // Try to make multiple requests to verify server is responsive
  for (int i = 0; i < 3; ++i) {
    res = client.Get("/status");
    EXPECT_NE(res, nullptr) << "Request " << i << " failed";
    if (res) {
      EXPECT_EQ(res->status, 200) << "Request " << i << " returned wrong status";
    }
    // Small delay between requests
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  server_->stop();
  waitForServerStopped();
}

// T1001: Server isRunning status
TEST_F(HttpServerTest, ServerIsRunningStatus) {
  EXPECT_FALSE(server_->isRunning());

  server_->setRootDirectory(test_web_dir_);
  server_->start(18100);

  waitForServerRunning();
  EXPECT_TRUE(server_->isRunning());

  server_->stop();
  waitForServerStopped();
  EXPECT_FALSE(server_->isRunning());
}

// T1001: Set root directory after start error
TEST_F(HttpServerTest, SetRootDirectoryAfterStartError) {
  server_->setRootDirectory(test_web_dir_);
  server_->start(18101);

  waitForServerRunning();

  // Try to set root directory while server is running (should fail)
  auto result = server_->setRootDirectory("another_dir");

  // This should return an error
  EXPECT_FALSE(result);

  server_->stop();
  waitForServerStopped();
}

} // namespace screensdk
