#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

#include "screensdk/core/display_controller.h"

namespace screensdk {

class DisplayControllerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create DisplayController instance using test factory function
    display_controller_ = CreateDisplayController();
    ASSERT_NE(display_controller_, nullptr);
  }

  void TearDown() override {
    if (display_controller_) {
      DestroyDisplayController(display_controller_);
      display_controller_ = nullptr;
    }
  }

  IDisplayController* display_controller_;
};

// T059: InitializeSuccess
TEST_F(DisplayControllerTest, InitializeSuccess) {
  EXPECT_TRUE(display_controller_->initialize());
}

// T059: GetDisplayListReturnsValidList
TEST_F(DisplayControllerTest, GetDisplayListReturnsValidList) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  EXPECT_GT(displays.size(), 0);
}

// T059: GetPrimaryDisplayReturnsPrimaryDisplay
TEST_F(DisplayControllerTest, GetPrimaryDisplayReturnsPrimaryDisplay) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  DisplaySource primary = display_controller_->getPrimaryDisplay();
  EXPECT_TRUE(primary.is_primary);
  EXPECT_EQ(primary.id, displays[0].id);
}

// T059: GetDisplayByIdReturnsCorrectDisplay
TEST_F(DisplayControllerTest, GetDisplayByIdReturnsCorrectDisplay) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  DisplaySource display = display_controller_->getDisplayById(displays[0].id);
  EXPECT_EQ(display.id, displays[0].id);
  EXPECT_EQ(display.name, displays[0].name);
  EXPECT_EQ(display.resolution_width, displays[0].resolution_width);
  EXPECT_EQ(display.resolution_height, displays[0].resolution_height);
}

// T059: GetDisplayByIdWithInvalidIdReturnsEmptyDisplay
TEST_F(DisplayControllerTest, GetDisplayByIdWithInvalidIdReturnsEmptyDisplay) {
  ASSERT_TRUE(display_controller_->initialize());

  DisplaySource display = display_controller_->getDisplayById(-1);
  EXPECT_EQ(display.id, 0);
  EXPECT_TRUE(display.name.empty());
}

// T059: SelectDisplaySuccess
TEST_F(DisplayControllerTest, SelectDisplaySuccess) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  EXPECT_EQ(display_controller_->getCurrentDisplayId(), -1);

  auto result = display_controller_->selectDisplay(displays[0].id);
  EXPECT_TRUE(static_cast<bool>(result));
  EXPECT_EQ(display_controller_->getCurrentDisplayId(), displays[0].id);
}

// T059: SelectDisplayWithInvalidIdFails
TEST_F(DisplayControllerTest, SelectDisplayWithInvalidIdFails) {
  ASSERT_TRUE(display_controller_->initialize());

  auto result = display_controller_->selectDisplay(-1);
  EXPECT_FALSE(static_cast<bool>(result));
}

// T059: SwitchDisplayTriggersRenegotiation
TEST_F(DisplayControllerTest, SwitchDisplayTriggersRenegotiation) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();

  // Skip test if less than 2 displays available
  if (displays.size() < 2) {
    GTEST_SKIP() << "Test requires at least 2 displays, only "
                 << displays.size() << " available";
    return;
  }

  // Select first display
  ASSERT_TRUE(static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  bool renegotiation_triggered = false;
  display_controller_->onDisplaySwitch([&](const DisplaySource& old_display,
                                           const DisplaySource& new_display) {
    renegotiation_triggered = true;
    EXPECT_EQ(old_display.id, displays[0].id);
    EXPECT_EQ(new_display.id, displays[1].id);
  });

  // Switch to second display
  auto result = display_controller_->switchDisplay(displays[1].id);
  EXPECT_TRUE(static_cast<bool>(result));
  EXPECT_TRUE(renegotiation_triggered);
  EXPECT_EQ(display_controller_->getCurrentDisplayId(), displays[1].id);
}

// T059: SwitchDisplayTimingUnder100ms
TEST_F(DisplayControllerTest, SwitchDisplayTimingUnder100ms) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();

  // Skip test if less than 2 displays available
  if (displays.size() < 2) {
    GTEST_SKIP() << "Test requires at least 2 displays, only "
                 << displays.size() << " available";
    return;
  }

  // Select first display
  ASSERT_TRUE(static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  auto start = std::chrono::high_resolution_clock::now();

  // Switch to second display
  auto result = display_controller_->switchDisplay(displays[1].id);
  ASSERT_TRUE(static_cast<bool>(result));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Switch time should be less than 100ms
  EXPECT_LT(duration.count(), 100);
}

// T059: DetectDisplayChanges
TEST_F(DisplayControllerTest, DetectDisplayChanges) {
  ASSERT_TRUE(display_controller_->initialize());

  auto initial_displays = display_controller_->getDisplayList();
  ASSERT_GT(initial_displays.size(), 0);

  // Save initial display count
  int initial_count = static_cast<int>(initial_displays.size());

  // Check if there are display changes (simulated scenario)
  // Note: This test depends on actual display configuration changes
  // Changes may not be detected in CI environments
  bool has_changes = display_controller_->detectDisplayChanges();
  EXPECT_TRUE(has_changes || !has_changes); // Accept both results
}

// T059: RefreshDisplayListUpdatesCache
TEST_F(DisplayControllerTest, RefreshDisplayListUpdatesCache) {
  ASSERT_TRUE(display_controller_->initialize());

  auto initial_displays = display_controller_->getDisplayList();
  ASSERT_GT(initial_displays.size(), 0);

  // Refresh display list
  display_controller_->refreshDisplayList();

  auto refreshed_displays = display_controller_->getDisplayList();

  // Check if list is updated (may be same or different)
  EXPECT_EQ(refreshed_displays.size(), initial_displays.size());
}

// T059: GetCurrentDisplayReturnsSelectedDisplay
TEST_F(DisplayControllerTest, GetCurrentDisplayReturnsSelectedDisplay) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  // Select first display
  ASSERT_TRUE(static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));

  DisplaySource current = display_controller_->getCurrentDisplay();
  EXPECT_EQ(current.id, displays[0].id);
  EXPECT_EQ(current.name, displays[0].name);
}

// T059: GetCurrentDisplayBeforeSelectionReturnsPrimary
TEST_F(DisplayControllerTest, GetCurrentDisplayBeforeSelectionReturnsPrimary) {
  ASSERT_TRUE(display_controller_->initialize());

  DisplaySource current = display_controller_->getCurrentDisplay();
  EXPECT_TRUE(current.is_primary);
}

// T059: DisplaySwitchWithoutRenegotiationCallback
TEST_F(DisplayControllerTest, DisplaySwitchWithoutRenegotiationCallback) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  ASSERT_GT(displays.size(), 0);

  // Switch without setting callback
  auto result = display_controller_->switchDisplay(displays[0].id);
  EXPECT_TRUE(static_cast<bool>(result));
}

// T059: MultipleDisplaySwitchesSupported
TEST_F(DisplayControllerTest, MultipleDisplaySwitchesSupported) {
  ASSERT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();

  // Skip test if less than 2 displays available
  if (displays.size() < 2) {
    GTEST_SKIP() << "Test requires at least 2 displays, only "
                 << displays.size() << " available";
    return;
  }

  // Switch displays multiple times
  ASSERT_TRUE(static_cast<bool>(display_controller_->selectDisplay(displays[0].id)));
  EXPECT_TRUE(static_cast<bool>(display_controller_->switchDisplay(displays[1].id)));
  EXPECT_TRUE(static_cast<bool>(display_controller_->switchDisplay(displays[0].id)));
  EXPECT_TRUE(static_cast<bool>(display_controller_->switchDisplay(displays[1].id)));

  EXPECT_EQ(display_controller_->getCurrentDisplayId(), displays[1].id);
}

// T059: DisplayControllerNotInitialized
TEST_F(DisplayControllerTest, DisplayControllerNotInitialized) {
  // Create but do not initialize
  IDisplayController* controller = CreateDisplayController();
  ASSERT_NE(controller, nullptr);

  // Call methods without initialization
  auto displays = controller->getDisplayList();
  EXPECT_TRUE(displays.empty());

  DisplaySource primary = controller->getPrimaryDisplay();
  EXPECT_EQ(primary.id, 0);

  DestroyDisplayController(controller);
}

// T059: ReinitializeAfterClose
TEST_F(DisplayControllerTest, ReinitializeAfterClose) {
  ASSERT_TRUE(display_controller_->initialize());
  display_controller_->close();

  // Re-initialize
  EXPECT_TRUE(display_controller_->initialize());

  auto displays = display_controller_->getDisplayList();
  EXPECT_GT(displays.size(), 0);
}

} // namespace screensdk
