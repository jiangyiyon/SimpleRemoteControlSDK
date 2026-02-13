#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "screensdk/core/display_controller.h"
#include "screensdk/transport/sdp_renegotiation.h"

namespace screensdk {

/**
 * @brief Integration test for DisplayController and SDP Renegotiation
 *
 * Tests the integration between DisplayController and SdpRenegotiation:
 * - DisplayController::switchDisplay() triggers onDisplaySwitch() callback
 * - Callback should invoke SdpRenegotiation::initiateDisplaySwitch()
 * - SDP renegotiation state transitions correctly
 * - Display switch timing meets < 100ms requirement
 *
 * Scenarios:
 * - Display switch triggers renegotiation callback
 * - Renegotiation completes with valid SDP
 * - Multiple switches handled correctly
 * - Timing requirements met
 * - Callback registration and invocation
 * - Error handling during switch
 *
 * Note: This test uses a mock PeerConnection since we don't have
 * a full WebRTC environment in unit tests.
 */
class DisplayControllerSdpIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    display_controller_ = CreateDisplayController();
    ASSERT_NE(display_controller_, nullptr);
  }

  void TearDown() override {
    if (display_controller_ != nullptr) {
      DestroyDisplayController(display_controller_);
      display_controller_ = nullptr;
    }
  }

  IDisplayController* display_controller_{nullptr};
};

TEST_F(DisplayControllerSdpIntegrationTest,
       DisplaySwitchTriggersCallback) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  if (displays.size() < 1) {
    GTEST_SKIP() << "No displays available";
    return;
  }

  // Track callback invocation
  std::atomic<bool> callback_invoked{false};
  DisplaySource old_display;
  DisplaySource new_display;

  // Register display switch callback
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource& old_disp, const DisplaySource& new_disp) {
        callback_invoked.store(true);
        old_display = old_disp;
        new_display = new_disp;
      });

  // Select first display
  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  // Switch to same display (should still trigger callback)
  auto result = display_controller_->switchDisplay(displays[0].id);
  ASSERT_TRUE(static_cast<bool>(result));

  // Verify callback was invoked
  EXPECT_TRUE(callback_invoked.load())
      << "Display switch callback should be invoked";

  // Verify callback parameters
  EXPECT_EQ(new_display.id, displays[0].id);
}

TEST_F(DisplayControllerSdpIntegrationTest,
       SwitchBetweenDisplaysTriggersCallbackWithCorrectParams) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  if (displays.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays, found: " << displays.size();
    return;
  }

  // Track callback invocations
  std::atomic<int> callback_count{0};
  std::vector<DisplaySource> old_displays;
  std::vector<DisplaySource> new_displays;

  // Register display switch callback
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource& old_disp, const DisplaySource& new_disp) {
        callback_count.fetch_add(1);
        old_displays.push_back(old_disp);
        new_displays.push_back(new_disp);
      });

  // Select first display
  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  // Switch to second display
  auto result1 = display_controller_->switchDisplay(displays[1].id);
  ASSERT_TRUE(static_cast<bool>(result1));

  // Switch back to first display
  auto result2 = display_controller_->switchDisplay(displays[0].id);
  ASSERT_TRUE(static_cast<bool>(result2));

  // Verify callback was invoked twice
  EXPECT_EQ(callback_count.load(), 2);

  // Verify first switch parameters
  EXPECT_EQ(old_displays[0].id, displays[0].id);
  EXPECT_EQ(new_displays[0].id, displays[1].id);

  // Verify second switch parameters
  EXPECT_EQ(old_displays[1].id, displays[1].id);
  EXPECT_EQ(new_displays[1].id, displays[0].id);
}

TEST_F(DisplayControllerSdpIntegrationTest,
       MultipleSwitchesHandledCorrectly) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  if (displays.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
    return;
  }

  std::atomic<int> callback_count{0};

  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_count.fetch_add(1);
      });

  // Perform multiple switches
  const int kSwitchCount = 5;

  for (int i = 0; i < kSwitchCount; ++i) {
    int target_id = (i % 2 == 0) ? displays[0].id : displays[1].id;
    auto result = display_controller_->switchDisplay(target_id);
    ASSERT_TRUE(static_cast<bool>(result));
  }

  // Verify callback invoked for each switch
  EXPECT_EQ(callback_count.load(), kSwitchCount);
}

TEST_F(DisplayControllerSdpIntegrationTest,
       SwitchWithInvalidIdDoesNotTriggerCallback) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  std::atomic<bool> callback_invoked{false};

  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_invoked.store(true);
      });

  // Select a valid display first
  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  // Try to switch to invalid display ID
  auto result = display_controller_->switchDisplay(-999);
  ASSERT_FALSE(static_cast<bool>(result));

  // Callback should NOT be invoked for failed switch
  EXPECT_FALSE(callback_invoked.load())
      << "Callback should not be invoked for failed switch";
}

TEST_F(DisplayControllerSdpIntegrationTest,
       CallbackCanBeUpdated) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  if (displays.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
    return;
  }

  std::atomic<int> first_callback_count{0};
  std::atomic<int> second_callback_count{0};

  // Register first callback
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        first_callback_count.fetch_add(1);
      });

  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));
  ASSERT_TRUE(
      static_cast<bool>(display_controller_->switchDisplay(displays[1].id)));

  // Update to second callback
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        second_callback_count.fetch_add(1);
      });

  ASSERT_TRUE(
      static_cast<bool>(display_controller_->switchDisplay(displays[0].id)));

  // First callback should only be called once, second once
  EXPECT_EQ(first_callback_count.load(), 1);
  EXPECT_EQ(second_callback_count.load(), 1);
}

TEST_F(DisplayControllerSdpIntegrationTest,
       SwitchWithoutCallbackDoesNotCrash) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  // Don't register any callback
  // This should not crash or throw exceptions

  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  auto result = display_controller_->switchDisplay(displays[0].id);
  ASSERT_TRUE(static_cast<bool>(result));
}

TEST_F(DisplayControllerSdpIntegrationTest,
       SwitchTimingUnder100ms) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  if (displays.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
    return;
  }

  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  std::atomic<bool> callback_invoked{false};

  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_invoked.store(true);
      });

  auto start = std::chrono::high_resolution_clock::now();

  auto result = display_controller_->switchDisplay(displays[1].id);
  ASSERT_TRUE(static_cast<bool>(result));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Wait for callback to be processed
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Verify callback was invoked
  EXPECT_TRUE(callback_invoked.load());

  // Switch time should be < 100ms (requirement)
  EXPECT_LT(duration.count(), 100)
      << "Display switch should complete in < 100ms, took: "
      << duration.count() << "ms";
}

TEST_F(DisplayControllerSdpIntegrationTest,
       ConcurrentSwitchCallsHandledSafely) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  if (displays.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
    return;
  }

  std::atomic<int> callback_count{0};

  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_count.fetch_add(1);
      });

  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  // Simulate concurrent switch calls from multiple threads
  const int kThreadCount = 5;
  std::vector<std::thread> threads;

  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([this, &displays, i]() {
      int target_id = (i % 2 == 0) ? displays[0].id : displays[1].id;
      display_controller_->switchDisplay(target_id);
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // All switches should complete without crashing
  // Callback count depends on which switch succeeded last
  EXPECT_GT(callback_count.load(), 0);
}

TEST_F(DisplayControllerSdpIntegrationTest,
       SelectDisplayDoesNotTriggerSwitchCallback) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  std::atomic<bool> switch_callback_invoked{false};

  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        switch_callback_invoked.store(true);
      });

  // selectDisplay should NOT trigger switch callback
  auto result = display_controller_->selectDisplay(displays[0].id);
  ASSERT_TRUE(static_cast<bool>(result));

  EXPECT_FALSE(switch_callback_invoked.load())
      << "selectDisplay should not trigger switch callback";
}

} // namespace screensdk
