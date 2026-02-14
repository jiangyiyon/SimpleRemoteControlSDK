#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>

#include "screensdk/core/session.h"

using namespace screensdk;

/**
 * @brief T033: Unit test for Session entity
 *
 * Tests Session class initialization, state transitions, latency tracking,
 * reconnection attempts, display source ID validation, and thread safety.
 */
class SessionTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

/**
 * @brief Test session initialization with default constructor
 */
TEST_F(SessionTest, Initialization) {
  Session session;

  EXPECT_EQ(session.getState(), SessionState::kDisconnected);
  EXPECT_FALSE(session.getSessionId().empty());
  EXPECT_EQ(session.getSessionId().length(), 36);  // UUID v4 format
  EXPECT_EQ(session.getDisplaySourceId(), -1);
  EXPECT_EQ(session.getLatencyMs(), 0);
  EXPECT_EQ(session.getReconnectionAttempts(), 0);
}

/**
 * @brief Test session ID generation is unique
 */
TEST_F(SessionTest, SessionIdGeneration) {
  Session session1;
  Session session2;
  Session session3;

  EXPECT_NE(session1.getSessionId(), session2.getSessionId());
  EXPECT_NE(session2.getSessionId(), session3.getSessionId());
  EXPECT_NE(session1.getSessionId(), session3.getSessionId());

  // Verify UUID v4 format (8-4-4-4-12 hex digits with dashes)
  std::string id = session1.getSessionId();
  EXPECT_EQ(id[8], '-');
  EXPECT_EQ(id[13], '-');
  EXPECT_EQ(id[18], '-');
  EXPECT_EQ(id[23], '-');
}

/**
 * @brief Test connection state transitions
 * kDisconnected -> kConnecting -> kConnected -> kDisconnected
 */
TEST_F(SessionTest, ConnectionStateTransitions) {
  Session session;

  // kDisconnected -> kConnecting
  EXPECT_TRUE(session.connect("192.168.1.100"));
  EXPECT_EQ(session.getState(), SessionState::kConnecting);
  EXPECT_EQ(session.getClientIp(), "192.168.1.100");

  // kConnecting -> kConnected
  EXPECT_TRUE(session.setConnected());
  EXPECT_EQ(session.getState(), SessionState::kConnected);

  // kConnected -> kDisconnected
  session.disconnect();
  EXPECT_EQ(session.getState(), SessionState::kDisconnected);
}

/**
 * @brief Test invalid state transitions are rejected
 */
TEST_F(SessionTest, InvalidStateTransition) {
  Session session;

  // Cannot setConnected from kDisconnected
  EXPECT_FALSE(session.setConnected());
  EXPECT_EQ(session.getState(), SessionState::kDisconnected);

  // Cannot connect from kConnected
  session.connect("192.168.1.100");
  session.setConnected();
  EXPECT_FALSE(session.connect("192.168.1.101"));  // Already connected
  EXPECT_EQ(session.getClientIp(), "192.168.1.100");  // IP unchanged

  // Cannot reconnect from kConnected (no error state)
  EXPECT_FALSE(session.reconnect());
}

/**
 * @brief Test latency tracking
 */
TEST_F(SessionTest, LatencyTracking) {
  Session session;

  EXPECT_EQ(session.getLatencyMs(), 0);

  session.updateLatency(50);
  EXPECT_EQ(session.getLatencyMs(), 50);

  session.updateLatency(75);
  EXPECT_EQ(session.getLatencyMs(), 75);

  session.updateLatency(0);  // Reset
  EXPECT_EQ(session.getLatencyMs(), 0);

  // Latency should stay within 0-1000ms range
  session.updateLatency(2000);  // Should clamp to 1000
  EXPECT_EQ(session.getLatencyMs(), 1000);
}

/**
 * @brief Test reconnection attempts tracking
 */
TEST_F(SessionTest, ReconnectionAttemptsTracking) {
  Session session;

  EXPECT_EQ(session.getReconnectionAttempts(), 0);

  session.incrementReconnectionAttempts();
  EXPECT_EQ(session.getReconnectionAttempts(), 1);

  session.incrementReconnectionAttempts();
  EXPECT_EQ(session.getReconnectionAttempts(), 2);

  session.resetReconnectionAttempts();
  EXPECT_EQ(session.getReconnectionAttempts(), 0);
}

/**
 * @brief Test display source ID validation (0-3)
 */
TEST_F(SessionTest, DisplaySourceIdValidation) {
  Session session;

  // Valid display IDs
  EXPECT_TRUE(session.setDisplaySourceId(0));
  EXPECT_EQ(session.getDisplaySourceId(), 0);

  EXPECT_TRUE(session.setDisplaySourceId(1));
  EXPECT_EQ(session.getDisplaySourceId(), 1);

  EXPECT_TRUE(session.setDisplaySourceId(2));
  EXPECT_EQ(session.getDisplaySourceId(), 2);

  EXPECT_TRUE(session.setDisplaySourceId(3));
  EXPECT_EQ(session.getDisplaySourceId(), 3);

  // Invalid display IDs
  EXPECT_FALSE(session.setDisplaySourceId(-1));
  EXPECT_EQ(session.getDisplaySourceId(), 3);  // Unchanged

  EXPECT_FALSE(session.setDisplaySourceId(4));
  EXPECT_EQ(session.getDisplaySourceId(), 3);  // Unchanged
}

/**
 * @brief Test thread safety of latency updates
 */
TEST_F(SessionTest, ThreadSafetyLatencyUpdates) {
  Session session;

  constexpr int kThreadCount = 10;
  constexpr int kUpdatesPerThread = 100;
  std::vector<std::thread> threads;

  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([&session, i]() {
      for (int j = 0; j < kUpdatesPerThread; ++j) {
        session.updateLatency(i * 10 + j);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // Final latency value should be in valid range (0-1000)
  int latency = session.getLatencyMs();
  EXPECT_GE(latency, 0);
  EXPECT_LE(latency, 1000);
}

/**
 * @brief Test thread safety of state changes
 */
TEST_F(SessionTest, ThreadSafetyStateChanges) {
  Session session;

  constexpr int kThreadCount = 5;
  std::vector<std::thread> threads;

  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([&session, i]() {
      if (i % 2 == 0) {
        session.connect("192.168.1.100");
        session.setConnected();
      } else {
        session.disconnect();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // Final state should be valid (no crash)
  EXPECT_TRUE(session.getState() == SessionState::kConnected ||
              session.getState() == SessionState::kDisconnected);
}

/**
 * @brief Test connection state history tracking
 */
TEST_F(SessionTest, ConnectionStateHistory) {
  Session session;

  auto history = session.getStateHistory();
  EXPECT_EQ(history.size(), 1);  // Initial kDisconnected state
  EXPECT_EQ(history[0], SessionState::kDisconnected);

  session.connect("192.168.1.100");
  history = session.getStateHistory();
  EXPECT_EQ(history.size(), 2);
  EXPECT_EQ(history[0], SessionState::kDisconnected);
  EXPECT_EQ(history[1], SessionState::kConnecting);

  session.setConnected();
  history = session.getStateHistory();
  EXPECT_EQ(history.size(), 3);
  EXPECT_EQ(history.back(), SessionState::kConnected);
}

/**
 * @brief Test activity timestamp update
 */
TEST_F(SessionTest, ActivityTimestampUpdate) {
  Session session;

  auto before_activity = session.getLastActivity();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  session.updateActivity();
  auto after_activity = session.getLastActivity();

  EXPECT_GT(after_activity, before_activity);
}

/**
 * @brief Test complete session lifecycle
 */
TEST_F(SessionTest, SessionLifecycle) {
  Session session;

  // kDisconnected -> kConnecting -> kConnected
  EXPECT_TRUE(session.connect("192.168.1.100"));
  EXPECT_TRUE(session.setConnected());

  // Set display source
  EXPECT_TRUE(session.setDisplaySourceId(1));
  EXPECT_EQ(session.getDisplaySourceId(), 1);

  // Update latency during session
  session.updateLatency(30);
  EXPECT_EQ(session.getLatencyMs(), 30);

  // Simulate reconnection
  session.setError();
  EXPECT_EQ(session.getState(), SessionState::kError);
  session.incrementReconnectionAttempts();
  EXPECT_EQ(session.getReconnectionAttempts(), 1);

  EXPECT_TRUE(session.reconnect());
  EXPECT_EQ(session.getState(), SessionState::kReconnecting);

  EXPECT_TRUE(session.setConnected());
  EXPECT_EQ(session.getState(), SessionState::kConnected);

  // Disconnect
  session.disconnect();
  EXPECT_EQ(session.getState(), SessionState::kDisconnected);
  EXPECT_EQ(session.getDisplaySourceId(), -1);  // Reset
}
