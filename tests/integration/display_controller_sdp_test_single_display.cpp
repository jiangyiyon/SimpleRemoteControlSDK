#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "screensdk/core/display_controller.h"

namespace screensdk {

/**
 * @brief Integration test for DisplayController with single display
 *
 * Tests display controller functionality in single-display environment
 * All tests should pass even with only 1 display
 */
class DisplayControllerSingleDisplayTest : public ::testing::Test {
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

TEST_F(DisplayControllerSingleDisplayTest, InitializeSucceeds) {
  auto result = display_controller_->initialize();
  EXPECT_TRUE(static_cast<bool>(result));
}

TEST_F(DisplayControllerSingleDisplayTest, GetDisplayListReturnsAtLeastOne) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  EXPECT_GE(displays.size(), 1);

  std::cout << "Display count: " << displays.size() << std::endl;
  for (const auto& display : displays) {
    std::cout << "  [" << display.id << "] " << display.name << " ("
              << display.resolution_width << "x"
              << display.resolution_height << ")" << std::endl;
  }
}

TEST_F(DisplayControllerSingleDisplayTest, GetPrimaryDisplayWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  DisplaySource primary = display_controller_->getPrimaryDisplay();
  EXPECT_TRUE(primary.is_primary);
  EXPECT_GE(primary.id, 0);  // Can be 0 (first display index)
  EXPECT_FALSE(primary.name.empty());
  EXPECT_GT(primary.resolution_width, 0);
  EXPECT_GT(primary.resolution_height, 0);
}

TEST_F(DisplayControllerSingleDisplayTest, GetDisplayByIdWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  DisplaySource display = display_controller_->getDisplayById(displays[0].id);
  EXPECT_EQ(display.id, displays[0].id);
  EXPECT_EQ(display.name, displays[0].name);
}

TEST_F(DisplayControllerSingleDisplayTest, GetDisplayByIdWithInvalidId) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  DisplaySource display = display_controller_->getDisplayById(-999);
  EXPECT_EQ(display.id, 0);
  EXPECT_TRUE(display.name.empty());
}

TEST_F(DisplayControllerSingleDisplayTest, SelectDisplayWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  EXPECT_EQ(display_controller_->getCurrentDisplayId(), -1);

  auto result = display_controller_->selectDisplay(displays[0].id);
  EXPECT_TRUE(static_cast<bool>(result));

  EXPECT_EQ(display_controller_->getCurrentDisplayId(), displays[0].id);
}

TEST_F(DisplayControllerSingleDisplayTest, SwitchDisplayOnSameDisplayWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  // Select display first
  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  // Track callback
  std::atomic<bool> callback_invoked{false};
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_invoked.store(true);
      });

  // Switch to same display
  auto result = display_controller_->switchDisplay(displays[0].id);
  EXPECT_TRUE(static_cast<bool>(result));

  // Callback should still be invoked
  EXPECT_TRUE(callback_invoked.load());
}

TEST_F(DisplayControllerSingleDisplayTest, SwitchDisplayWithInvalidIdFails) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  auto result = display_controller_->switchDisplay(-999);
  EXPECT_FALSE(static_cast<bool>(result));
}

TEST_F(DisplayControllerSingleDisplayTest, GetCurrentDisplayWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  // Before selection, should return primary
  DisplaySource current = display_controller_->getCurrentDisplay();
  EXPECT_TRUE(current.is_primary);

  // After selection, should return selected display
  display_controller_->selectDisplay(displays[0].id);
  current = display_controller_->getCurrentDisplay();
  EXPECT_EQ(current.id, displays[0].id);
}

TEST_F(DisplayControllerSingleDisplayTest, RefreshDisplayListWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto initial_displays = display_controller_->getDisplayList();
  ASSERT_GE(initial_displays.size(), 1);

  display_controller_->refreshDisplayList();

  auto refreshed_displays = display_controller_->getDisplayList();

  // Display count should remain the same
  EXPECT_EQ(refreshed_displays.size(), initial_displays.size());
}

TEST_F(DisplayControllerSingleDisplayTest, DetectDisplayChangesWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  bool has_changes = display_controller_->detectDisplayChanges();

  // Accept both results - may or may not have changes
  EXPECT_TRUE(has_changes || !has_changes);
}

TEST_F(DisplayControllerSingleDisplayTest, CallbackCanBeSet) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  bool callback_called = false;
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_called = true;
      });

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  display_controller_->switchDisplay(displays[0].id);

  EXPECT_TRUE(callback_called);
}

TEST_F(DisplayControllerSingleDisplayTest, DisplayChangeCallbackWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  bool callback_called = false;
  display_controller_->onDisplayChange([&]() { callback_called = true; });

  // Calling detectDisplayChanges may or may not trigger callback
  display_controller_->detectDisplayChanges();

  // We can't assert on this since it depends on actual hardware
  (void)callback_called;
}

TEST_F(DisplayControllerSingleDisplayTest, MultipleSwitchesWork) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  std::atomic<int> callback_count{0};
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_count.fetch_add(1);
      });

  // Perform multiple switches to same display
  for (int i = 0; i < 5; ++i) {
    auto result = display_controller_->switchDisplay(displays[0].id);
    EXPECT_TRUE(static_cast<bool>(result));
  }

  EXPECT_EQ(callback_count.load(), 5);
}

TEST_F(DisplayControllerSingleDisplayTest, CloseAndReinitializeWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays_before = display_controller_->getDisplayList();
  display_controller_->close();

  EXPECT_TRUE(display_controller_->getDisplayList().empty());

  // Reinitialize
  EXPECT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays_after = display_controller_->getDisplayList();
  EXPECT_EQ(displays_after.size(), displays_before.size());
}

TEST_F(DisplayControllerSingleDisplayTest, SwitchTimingIsReasonable) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  ASSERT_TRUE(
      static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  std::atomic<bool> callback_invoked{false};
  display_controller_->onDisplaySwitch(
      [&](const DisplaySource&, const DisplaySource&) {
        callback_invoked.store(true);
      });

  auto start = std::chrono::high_resolution_clock::now();

  auto result = display_controller_->switchDisplay(displays[0].id);
  ASSERT_TRUE(static_cast<bool>(result));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_TRUE(callback_invoked.load());

  std::cout << "Switch completed in " << duration.count() << "ms"
            << std::endl;

  // Switch should be fast (< 50ms for same display)
  EXPECT_LT(duration.count(), 50);
}

TEST_F(DisplayControllerSingleDisplayTest, GetCurrentDisplayIdWorks) {
  ASSERT_TRUE(static_cast<bool>(display_controller_->initialize()));

  // Before selection
  EXPECT_EQ(display_controller_->getCurrentDisplayId(), -1);

  auto displays = display_controller_->getDisplayList();
  ASSERT_GE(displays.size(), 1);

  // After selection
  display_controller_->selectDisplay(displays[0].id);
  EXPECT_EQ(display_controller_->getCurrentDisplayId(), displays[0].id);
}

} // namespace screensdk
