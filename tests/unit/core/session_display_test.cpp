#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <unordered_map>

#include "screensdk/core/display_controller.h"
#include "screensdk/core/session.h"

using namespace screensdk;

/**
 * @brief T061: Unit test for display selection per session
 *
 * Tests DisplayController session-aware display selection methods:
 * - selectDisplayForSession
 * - switchDisplayForSession
 * - getDisplayForSession
 */
class SessionDisplayTest : public ::testing::Test {
protected:
  void SetUp() override {
    controller_ = std::unique_ptr<IDisplayController>(CreateDisplayController());
    ASSERT_TRUE(controller_->initialize().has_value());
  }

  void TearDown() override {
    controller_->close();
  }

  std::unique_ptr<IDisplayController> controller_;
};

/**
 * @brief Test selecting display for session with valid ID
 */
TEST_F(SessionDisplayTest, SelectDisplayForSessionValidId) {
  Session session;
  session.connect("192.168.1.100");
  session.setConnected();

  std::string session_id = session.getSessionId();
  int display_count = controller_->getDisplayList().size();

  if (display_count > 0) {
    auto display_list = controller_->getDisplayList();
    int display_id = display_list[0].id;

    auto result = controller_->selectDisplayForSession(session_id, display_id);
    EXPECT_TRUE(result.has_value());

    auto selected_display = controller_->getDisplayForSession(session_id);
    EXPECT_TRUE(selected_display.has_value());
    EXPECT_EQ(selected_display->id, display_id);
  } else {
    GTEST_SKIP() << "No displays available for testing";
  }
}

/**
 * @brief Test selecting display for session with invalid ID
 */
TEST_F(SessionDisplayTest, SelectDisplayForSessionInvalidId) {
  Session session;
  session.connect("192.168.1.100");
  session.setConnected();

  std::string session_id = session.getSessionId();

  auto result = controller_->selectDisplayForSession(session_id, -1);
  EXPECT_FALSE(result.has_value());

  result = controller_->selectDisplayForSession(session_id, 4);
  EXPECT_FALSE(result.has_value());

  auto selected_display = controller_->getDisplayForSession(session_id);
  EXPECT_FALSE(selected_display.has_value());
}

/**
 * @brief Test multiple sessions can select different displays
 */
TEST_F(SessionDisplayTest, MultipleSessionsDifferentDisplays) {
  Session session1;
  Session session2;
  session1.connect("192.168.1.100");
  session2.connect("192.168.1.101");
  session1.setConnected();
  session2.setConnected();

  auto display_list = controller_->getDisplayList();

  if (display_list.size() >= 2) {
    std::string session_id1 = session1.getSessionId();
    std::string session_id2 = session2.getSessionId();
    int display_id1 = display_list[0].id;
    int display_id2 = display_list[1].id;

    auto result1 = controller_->selectDisplayForSession(session_id1, display_id1);
    auto result2 = controller_->selectDisplayForSession(session_id2, display_id2);

    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());

    auto display1 = controller_->getDisplayForSession(session_id1);
    auto display2 = controller_->getDisplayForSession(session_id2);

    EXPECT_TRUE(display1.has_value());
    EXPECT_TRUE(display2.has_value());
    EXPECT_EQ(display1->id, display_id1);
    EXPECT_EQ(display2->id, display_id2);
    EXPECT_NE(display1->id, display2->id);
  } else {
    GTEST_SKIP() << "Need at least 2 displays for testing";
  }
}

/**
 * @brief Test switching display for session triggers callback
 */
TEST_F(SessionDisplayTest, SwitchDisplayForSessionTriggersCallback) {
  Session session;
  session.connect("192.168.1.100");
  session.setConnected();

  auto display_list = controller_->getDisplayList();

  if (display_list.size() >= 2) {
    std::string session_id = session.getSessionId();
    int display_id1 = display_list[0].id;
    int display_id2 = display_list[1].id;

    // Select initial display
    controller_->selectDisplayForSession(session_id, display_id1);

    bool callback_triggered = false;
    DisplaySource old_display, new_display;

    controller_->onDisplaySwitch([&callback_triggered, &old_display, &new_display](
                                     const DisplaySource& old_disp,
                                     const DisplaySource& new_disp) {
      callback_triggered = true;
      old_display = old_disp;
      new_display = new_disp;
    });

    // Switch display
    auto result = controller_->switchDisplayForSession(session_id, display_id2);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(callback_triggered);
    EXPECT_EQ(old_display.id, display_id1);
    EXPECT_EQ(new_display.id, display_id2);
  } else {
    GTEST_SKIP() << "Need at least 2 displays for testing";
  }
}

/**
 * @brief Test display switch timing under 100ms
 */
TEST_F(SessionDisplayTest, SwitchDisplayForSessionTiming) {
  Session session;
  session.connect("192.168.1.100");
  session.setConnected();

  auto display_list = controller_->getDisplayList();

  if (display_list.size() >= 2) {
    std::string session_id = session.getSessionId();
    int display_id1 = display_list[0].id;
    int display_id2 = display_list[1].id;

    controller_->selectDisplayForSession(session_id, display_id1);

    auto start_time = std::chrono::high_resolution_clock::now();
    auto result = controller_->switchDisplayForSession(session_id, display_id2);
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time - start_time)
                        .count();

    EXPECT_TRUE(result.has_value());
    EXPECT_LT(duration, 100);  // Switch time < 100ms
  } else {
    GTEST_SKIP() << "Need at least 2 displays for testing";
  }
}

/**
 * @brief Test getting display for session
 */
TEST_F(SessionDisplayTest, GetDisplayForSession) {
  Session session;
  session.connect("192.168.1.100");
  session.setConnected();

  auto display_list = controller_->getDisplayList();

  if (display_list.size() > 0) {
    std::string session_id = session.getSessionId();
    int display_id = display_list[0].id;

    auto result = controller_->selectDisplayForSession(session_id, display_id);
    EXPECT_TRUE(result.has_value());

    auto selected_display = controller_->getDisplayForSession(session_id);
    EXPECT_TRUE(selected_display.has_value());
    EXPECT_EQ(selected_display->id, display_id);
    EXPECT_EQ(selected_display->name, display_list[0].name);
  } else {
    GTEST_SKIP() << "No displays available for testing";
  }
}

/**
 * @brief Test session display state persistence
 */
TEST_F(SessionDisplayTest, SessionDisplayStatePersistence) {
  Session session;
  session.connect("192.168.1.100");
  session.setConnected();

  auto display_list = controller_->getDisplayList();

  if (display_list.size() > 0) {
    std::string session_id = session.getSessionId();
    int display_id = display_list[0].id;

    controller_->selectDisplayForSession(session_id, display_id);

    // Verify display selection persists
    auto display1 = controller_->getDisplayForSession(session_id);
    EXPECT_TRUE(display1.has_value());
    EXPECT_EQ(display1->id, display_id);

    // Select again and verify
    controller_->selectDisplayForSession(session_id, display_id);
    auto display2 = controller_->getDisplayForSession(session_id);
    EXPECT_TRUE(display2.has_value());
    EXPECT_EQ(display2->id, display_id);
  } else {
    GTEST_SKIP() << "No displays available for testing";
  }
}

/**
 * @brief Test handling of invalid session ID
 */
TEST_F(SessionDisplayTest, InvalidSessionHandling) {
  std::string invalid_session_id = "INVALID-SESSION-ID-12345";

  auto result = controller_->selectDisplayForSession(invalid_session_id, 0);
  EXPECT_FALSE(result);  // Should fail (invalid session ID)

  auto result_switch = controller_->switchDisplayForSession(invalid_session_id, 0);
  EXPECT_FALSE(result_switch);  // Should fail (invalid session ID)

  auto display = controller_->getDisplayForSession(invalid_session_id);
  EXPECT_FALSE(display.has_value());  // Should return empty (session not in map)
}
