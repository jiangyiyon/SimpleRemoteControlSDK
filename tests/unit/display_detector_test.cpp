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

// T055: Unit test for DisplaySource entity
TEST_F(DisplayDetectorTest, DisplaySourceInitialization) {
  auto displays = detector_.getDisplays();

  if (!displays.empty()) {
    DisplaySource source;
    source.id = displays[0].index;
    source.name = displays[0].name;
    source.resolution_width = displays[0].width;
    source.resolution_height = displays[0].height;
    source.refresh_rate = displays[0].refresh_rate;
    source.is_primary = displays[0].is_primary;
    source.is_active = false;
    source.capture_handle = nullptr;

    EXPECT_EQ(source.id, displays[0].index);
    EXPECT_EQ(source.name, displays[0].name);
    EXPECT_EQ(source.resolution_width, displays[0].width);
    EXPECT_EQ(source.resolution_height, displays[0].height);
    EXPECT_EQ(source.refresh_rate, displays[0].refresh_rate);
    EXPECT_EQ(source.is_primary, displays[0].is_primary);
    EXPECT_FALSE(source.is_active.load());
    EXPECT_EQ(source.capture_handle, nullptr);
  }
}

TEST_F(DisplayDetectorTest, DisplaySourceIdRange) {
  auto displays = detector_.getDisplays();

  for (size_t i = 0; i < displays.size(); ++i) {
    EXPECT_GE(displays[i].index, 0);
    EXPECT_LT(displays[i].index, 4);  // Max 4 displays
  }
}

TEST_F(DisplayDetectorTest, DisplaySourceResolutionConstraints) {
  auto displays = detector_.getDisplays();

  for (const auto& display : displays) {
    EXPECT_GE(display.width, 640);
    EXPECT_LE(display.width, 7680);   // 4K max
    EXPECT_GE(display.height, 480);
    EXPECT_LE(display.height, 4320);
  }
}

TEST_F(DisplayDetectorTest, DisplaySourceRefreshRateConstraints) {
  auto displays = detector_.getDisplays();

  for (const auto& display : displays) {
    EXPECT_GE(display.refresh_rate, 30);
    EXPECT_LE(display.refresh_rate, 240);
  }
}

TEST_F(DisplayDetectorTest, DisplaySourceNameMaxLength) {
  auto displays = detector_.getDisplays();

  for (const auto& display : displays) {
    EXPECT_LE(display.name.length(), 256);
  }
}

TEST_F(DisplayDetectorTest, DisplaySourceAtomicActiveState) {
  DisplaySource source;
  source.is_active = false;

  EXPECT_FALSE(source.is_active.load());

  source.is_active.store(true);
  EXPECT_TRUE(source.is_active.load());

  source.is_active.store(false);
  EXPECT_FALSE(source.is_active.load());
}

TEST_F(DisplayDetectorTest, DisplaySourceOnlyOnePrimary) {
  auto displays = detector_.getDisplays();
  int primary_count = 0;

  for (const auto& display : displays) {
    if (display.is_primary) {
      ++primary_count;
    }
  }

  EXPECT_EQ(primary_count, 1);
}

TEST_F(DisplayDetectorTest, DisplaySourceHandleNullInitially) {
  DisplaySource source;
  source.capture_handle = nullptr;

  EXPECT_EQ(source.capture_handle, nullptr);
}
