/**
 * @file screensdk_api_test.cpp
 * @brief Unit tests for public API
 *
 * T047: Implement screensdk::createSession() public API
 *
 * Tests the public C-style API for session creation, display controller,
 * and video source factory functions.
 */

#include <gtest/gtest.h>
#include <screensdk/screensdk.h>
#include <screensdk/core/display_controller.h>

using namespace screensdk;

/**
 * @brief Test suite for screensdk public API
 */
class ScreensdkApiTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Reset any global state before each test
  }

  void TearDown() override {
    // Clean up any global state after each test
  }
};

/**
 * @brief Test CreateSession factory function
 */
TEST_F(ScreensdkApiTest, CreateSessionReturnsValidInstance) {
  ISession* session = CreateSession();

  ASSERT_NE(session, nullptr) << "CreateSession should return non-null instance";
  EXPECT_FALSE(session->getSessionId().empty()) << "Session ID should not be empty";
  EXPECT_EQ(session->getState(), 0)  // SessionState::kDisconnected
      << "Initial state should be kDisconnected";

  DestroySession(session);
}

/**
 * @brief Test CreateSession multiple calls create unique sessions
 */
TEST_F(ScreensdkApiTest, CreateMultipleSessionsHaveUniqueIds) {
  ISession* session1 = CreateSession();
  ISession* session2 = CreateSession();

  ASSERT_NE(session1, nullptr);
  ASSERT_NE(session2, nullptr);

  std::string id1 = session1->getSessionId();
  std::string id2 = session2->getSessionId();

  EXPECT_NE(id1, id2) << "Multiple sessions should have unique IDs";

  DestroySession(session1);
  DestroySession(session2);
}

/**
 * @brief Test DestroySession with nullptr is safe
 */
TEST_F(ScreensdkApiTest, DestroySessionWithNullptrIsSafe) {
  EXPECT_NO_THROW(DestroySession(nullptr));
}

/**
 * @brief Test session connect and state transitions
 */
TEST_F(ScreensdkApiTest, SessionConnectTransitionsState) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  EXPECT_EQ(session->getState(), 0);  // SessionState::kDisconnected

  int result = session->connect("192.168.1.100");
  EXPECT_EQ(result, 0) << "connect() should succeed";
  EXPECT_EQ(session->getState(), 1);  // SessionState::kConnecting

  result = session->setConnected();
  EXPECT_EQ(result, 0) << "setConnected() should succeed";
  EXPECT_EQ(session->getState(), 2);  // SessionState::kConnected

  DestroySession(session);
}

/**
 * @brief Test session disconnect
 */
TEST_F(ScreensdkApiTest, SessionDisconnectTransitionsState) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  session->connect("192.168.1.100");
  session->setConnected();
  EXPECT_EQ(session->getState(), 2);  // SessionState::kConnected

  session->disconnect();
  EXPECT_EQ(session->getState(), 0);  // SessionState::kDisconnected

  DestroySession(session);
}

/**
 * @brief Test session client IP tracking
 */
TEST_F(ScreensdkApiTest, SessionTracksClientIp) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  std::string client_ip = "192.168.1.100";
  session->connect(client_ip);

  EXPECT_EQ(session->getClientIp(), client_ip);

  session->disconnect();
  EXPECT_TRUE(session->getClientIp().empty()) << "Client IP should be cleared on disconnect";

  DestroySession(session);
}

/**
 * @brief Test session display source ID
 */
TEST_F(ScreensdkApiTest, SessionSetAndGetDisplaySourceId) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  EXPECT_EQ(session->getDisplaySourceId(), -1) << "Initial display ID should be -1";

  int result = session->setDisplaySourceId(2);
  EXPECT_EQ(result, 0) << "setDisplaySourceId(2) should succeed";
  EXPECT_EQ(session->getDisplaySourceId(), 2);

  result = session->setDisplaySourceId(0);
  EXPECT_EQ(result, 0) << "setDisplaySourceId(0) should succeed";
  EXPECT_EQ(session->getDisplaySourceId(), 0);

  // Test invalid display ID (out of range 0-3)
  result = session->setDisplaySourceId(5);
  EXPECT_NE(result, 0) << "setDisplaySourceId(5) should fail";

  DestroySession(session);
}

/**
 * @brief Test session latency tracking
 */
TEST_F(ScreensdkApiTest, SessionLatencyTracking) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  EXPECT_EQ(session->getLatencyMs(), 0) << "Initial latency should be 0";

  session->updateLatency(50);
  EXPECT_EQ(session->getLatencyMs(), 50);

  session->updateLatency(100);
  EXPECT_EQ(session->getLatencyMs(), 100);

  // Test clamping to 1000
  session->updateLatency(2000);
  EXPECT_EQ(session->getLatencyMs(), 1000) << "Latency should be clamped to 1000";

  // Test clamping to 0
  session->updateLatency(-100);
  EXPECT_EQ(session->getLatencyMs(), 0) << "Latency should be clamped to 0";

  DestroySession(session);
}

/**
 * @brief Test session reconnection attempts
 */
TEST_F(ScreensdkApiTest, SessionReconnectionAttempts) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  EXPECT_EQ(session->getReconnectionAttempts(), 0);

  session->incrementReconnectionAttempts();
  EXPECT_EQ(session->getReconnectionAttempts(), 1);

  session->incrementReconnectionAttempts();
  session->incrementReconnectionAttempts();
  EXPECT_EQ(session->getReconnectionAttempts(), 3);

  session->resetReconnectionAttempts();
  EXPECT_EQ(session->getReconnectionAttempts(), 0);

  DestroySession(session);
}

/**
 * @brief Test session state history
 */
TEST_F(ScreensdkApiTest, SessionStateHistoryTracking) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  auto history = session->getStateHistory();
  EXPECT_EQ(history.size(), 1) << "Initial history should have one state";
  EXPECT_EQ(history[0], 0);  // SessionState::kDisconnected

  session->connect("192.168.1.100");
  history = session->getStateHistory();
  EXPECT_EQ(history.size(), 2);
  EXPECT_EQ(history.back(), 1);  // SessionState::kConnecting

  session->setConnected();
  history = session->getStateHistory();
  EXPECT_EQ(history.size(), 3);
  EXPECT_EQ(history.back(), 2);  // SessionState::kConnected

  DestroySession(session);
}

/**
 * @brief Test session error state
 */
TEST_F(ScreensdkApiTest, SessionErrorState) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  session->setError();
  EXPECT_EQ(session->getState(), 4);  // SessionState::kError

  DestroySession(session);
}

/**
 * @brief Test session reconnect
 */
TEST_F(ScreensdkApiTest, SessionReconnectFromError) {
  ISession* session = CreateSession();
  ASSERT_NE(session, nullptr);

  session->connect("192.168.1.100");
  session->setConnected();
  session->setError();

  int result = session->reconnect();
  EXPECT_EQ(result, 0) << "reconnect() should succeed";
  EXPECT_EQ(session->getState(), 3);  // SessionState::kReconnecting

  DestroySession(session);
}

/**
 * @brief Test CreateDisplayController factory function
 */
TEST_F(ScreensdkApiTest, CreateDisplayControllerReturnsValidInstance) {
  IDisplayController* controller = CreateDisplayController();

  ASSERT_NE(controller, nullptr) << "CreateDisplayController should return non-null instance";

  // Initialize controller before use
  auto init_result = controller->initialize();
  EXPECT_TRUE(init_result) << "initialize() should succeed";

  // Test basic operations
  int display_id = controller->getCurrentDisplayId();
  EXPECT_GE(display_id, -1) << "Display ID should be >= -1";
  EXPECT_LE(display_id, 3) << "Display ID should be <= 3";

  auto displays = controller->getDisplayList();
  EXPECT_GT(displays.size(), 0) << "Should have at least one display";

  controller->close();
  DestroyDisplayController(controller);
}

/**
 * @brief Test DestroyDisplayController with nullptr is safe
 */
TEST_F(ScreensdkApiTest, DestroyDisplayControllerWithNullptrIsSafe) {
  EXPECT_NO_THROW(DestroyDisplayController(nullptr));
}

/**
 * @brief Test GetSDKVersion returns valid version
 */
TEST_F(ScreensdkApiTest, GetSDKVersionReturnsValidString) {
  const char* version = GetSDKVersion();

  ASSERT_NE(version, nullptr) << "GetSDKVersion should return non-null";
  EXPECT_STRNE(version, "") << "Version string should not be empty";

  // Check format: major.minor.patch
  std::string version_str(version);
  size_t dot_count = std::count(version_str.begin(), version_str.end(), '.');
  EXPECT_EQ(dot_count, 2) << "Version should be in format major.minor.patch";
}

/**
 * @brief Test GetSDKBuildInfo returns valid string
 */
TEST_F(ScreensdkApiTest, GetSDKBuildInfoReturnsValidString) {
  const char* build_info = GetSDKBuildInfo();

  ASSERT_NE(build_info, nullptr) << "GetSDKBuildInfo should return non-null";
  EXPECT_STRNE(build_info, "") << "Build info string should not be empty";
}

/**
 * @brief Test display controller get primary display
 */
TEST_F(ScreensdkApiTest, DisplayControllerGetPrimaryDisplay) {
  IDisplayController* controller = CreateDisplayController();
  ASSERT_NE(controller, nullptr);

  // Initialize controller before use
  auto init_result = controller->initialize();
  EXPECT_TRUE(init_result) << "initialize() should succeed";

  DisplaySource primary = controller->getPrimaryDisplay();

  EXPECT_TRUE(primary.is_primary) << "Primary display should have is_primary=true";
  EXPECT_GT(primary.resolution_width, 0) << "Primary display should have resolution_width > 0";
  EXPECT_GT(primary.resolution_height, 0) << "Primary display should have resolution_height > 0";
  EXPECT_GT(primary.refresh_rate, 0) << "Primary display should have refresh_rate > 0";

  controller->close();
  DestroyDisplayController(controller);
}

/**
 * @brief Test display controller get display by ID
 */
TEST_F(ScreensdkApiTest, DisplayControllerGetDisplayById) {
  IDisplayController* controller = CreateDisplayController();
  ASSERT_NE(controller, nullptr);

  // Initialize controller before use
  auto init_result = controller->initialize();
  EXPECT_TRUE(init_result) << "initialize() should succeed";

  // Try to get primary display by ID
  DisplaySource primary = controller->getPrimaryDisplay();
  DisplaySource by_id = controller->getDisplayById(primary.id);

  EXPECT_EQ(by_id.id, primary.id);
  EXPECT_EQ(by_id.name, primary.name);
  EXPECT_EQ(by_id.resolution_width, primary.resolution_width);
  EXPECT_EQ(by_id.resolution_height, primary.resolution_height);

  // Try invalid ID
  DisplaySource invalid = controller->getDisplayById(999);
  EXPECT_EQ(invalid.id, 0) << "Invalid display ID should return display with id=0";

  controller->close();
  DestroyDisplayController(controller);
}

/**
 * @brief Test display controller select and switch display
 */
TEST_F(ScreensdkApiTest, DisplayControllerSelectAndSwitchDisplay) {
  IDisplayController* controller = CreateDisplayController();
  ASSERT_NE(controller, nullptr);

  // Initialize controller before use
  auto init_result = controller->initialize();
  EXPECT_TRUE(init_result) << "initialize() should succeed";

  auto displays = controller->getDisplayList();
  if (displays.size() > 0) {
    int display_id = displays[0].id;

    auto result = controller->selectDisplay(display_id);
    EXPECT_TRUE(result) << "selectDisplay should succeed";
    EXPECT_EQ(controller->getCurrentDisplayId(), display_id);

    auto switch_result = controller->switchDisplay(display_id);
    // Note: switchDisplay may fail if SDP renegotiation callback is not set
    // We just check that it doesn't crash
    EXPECT_GE(switch_result.has_value(), true);
  }

  controller->close();
  DestroyDisplayController(controller);
}
