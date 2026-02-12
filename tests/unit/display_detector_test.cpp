#include <gtest/gtest.h>
#include "screensdk/capture/display_detector.h"

using namespace screensdk;

class DisplayDetectorTest : public ::testing::Test {
protected:
  DisplayDetector detector_;
};

TEST_F(DisplayDetectorTest, GetDisplaysReturnsNonEmpty) {
  auto displays = detector_.getDisplays();

  ASSERT_FALSE(displays.empty());
}

TEST_F(DisplayDetectorTest, GetDisplayCountPositive) {
  int count = detector_.getDisplayCount();

  ASSERT_GT(count, 0);
}

TEST_F(DisplayDetectorTest, GetPrimaryDisplay) {
  auto primary = detector_.getPrimaryDisplay();

  EXPECT_GT(primary.width, 0);
  EXPECT_GT(primary.height, 0);
  EXPECT_GT(primary.refresh_rate, 0);
  EXPECT_TRUE(primary.is_primary);
}

TEST_F(DisplayDetectorTest, GetDisplayByIndex) {
  auto displays = detector_.getDisplays();

  if (!displays.empty()) {
    auto display = detector_.getDisplay(0);

    EXPECT_EQ(display.index, 0);
    EXPECT_GT(display.width, 0);
    EXPECT_GT(display.height, 0);
  }
}

TEST_F(DisplayDetectorTest, DisplayDimensionsValid) {
  auto displays = detector_.getDisplays();

  for (const auto& display : displays) {
    EXPECT_GT(display.width, 0);
    EXPECT_GT(display.height, 0);
    EXPECT_GT(display.refresh_rate, 30);  // Minimum 30Hz
    EXPECT_LE(display.width, 7680);       // 8K max
    EXPECT_LE(display.height, 4320);
  }
}

TEST_F(DisplayDetectorTest, PrimaryDisplayExists) {
  auto displays = detector_.getDisplays();
  bool has_primary = false;

  for (const auto& display : displays) {
    if (display.is_primary) {
      has_primary = true;
      break;
    }
  }

  EXPECT_TRUE(has_primary);
}

TEST_F(DisplayDetectorTest, DisplayNameNotEmpty) {
  auto displays = detector_.getDisplays();

  for (const auto& display : displays) {
    EXPECT_FALSE(display.name.empty());
  }
}

TEST_F(DisplayDetectorTest, DesktopRectValid) {
  auto displays = detector_.getDisplays();

  for (const auto& display : displays) {
    EXPECT_LE(display.desktop_rect.left, display.desktop_rect.right);
    EXPECT_LE(display.desktop_rect.top, display.desktop_rect.bottom);
    EXPECT_GE(display.width, display.desktop_rect.right - display.desktop_rect.left);
    EXPECT_GE(display.height, display.desktop_rect.bottom - display.desktop_rect.top);
  }
}

TEST_F(DisplayDetectorTest, RefreshCache) {
  int count1 = detector_.getDisplayCount();
  detector_.refresh();
  int count2 = detector_.getDisplayCount();

  // Count should be consistent
  EXPECT_EQ(count1, count2);
}

TEST_F(DisplayDetectorTest, HasDisplayChangedInitialFalse) {
  // First call should return false
  EXPECT_FALSE(detector_.hasDisplayChanged());
}
