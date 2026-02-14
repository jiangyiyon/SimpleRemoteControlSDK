#include <gtest/gtest.h>
#include "screensdk/capture/display_detector.h"

namespace screensdk {

class DisplayDetectorDxgiTest : public ::testing::Test {
protected:
  void SetUp() override {
    detector_ = std::make_unique<DisplayDetector>();
  }

  void TearDown() override {
    detector_.reset();
  }

  std::unique_ptr<DisplayDetector> detector_;
};

// Test 1: GetDisplaySourcesReturnsNonEmpty
// Verify that getDisplaySources() returns at least one display
TEST_F(DisplayDetectorDxgiTest, GetDisplaySourcesReturnsNonEmpty) {
  auto sources = detector_->getDisplaySources();

  EXPECT_FALSE(sources.empty()) << "At least one display should be available";
}

// Test 2: DisplaySourceIdInRange
// Verify that display source IDs are within valid range (0-3)
TEST_F(DisplayDetectorDxgiTest, DisplaySourceIdInRange) {
  auto sources = detector_->getDisplaySources();

  for (const auto& source : sources) {
    EXPECT_GE(source.id, 0) << "Display source ID should be >= 0";
    EXPECT_LE(source.id, 3) << "Display source ID should be <= 3 (max 4 displays)";
  }
}

// Test 3: DisplaySourceHasValidResolution
// Verify that display source resolution is valid (>0)
TEST_F(DisplayDetectorDxgiTest, DisplaySourceHasValidResolution) {
  auto sources = detector_->getDisplaySources();

  for (const auto& source : sources) {
    EXPECT_GT(source.resolution_width, 0) << "Display source width should be > 0";
    EXPECT_GT(source.resolution_height, 0) << "Display source height should be > 0";
  }
}

// Test 4: DisplaySourceRefreshRateInRange
// Verify that display source refresh rate is within valid range (30-240 Hz)
TEST_F(DisplayDetectorDxgiTest, DisplaySourceRefreshRateInRange) {
  auto sources = detector_->getDisplaySources();

  for (const auto& source : sources) {
    EXPECT_GE(source.refresh_rate, 30) << "Display source refresh rate should be >= 30 Hz";
    EXPECT_LE(source.refresh_rate, 240) << "Display source refresh rate should be <= 240 Hz";
  }
}

// Test 5: ExactlyOnePrimaryDisplay
// Verify that exactly one display is marked as primary
TEST_F(DisplayDetectorDxgiTest, ExactlyOnePrimaryDisplay) {
  auto sources = detector_->getDisplaySources();

  int primary_count = 0;
  for (const auto& source : sources) {
    if (source.is_primary) {
      primary_count++;
    }
  }

  EXPECT_EQ(primary_count, 1) << "Exactly one display should be marked as primary";
}

// Test 6: DisplaySourceNameNotEmpty
// Verify that display source name is not empty
TEST_F(DisplayDetectorDxgiTest, DisplaySourceNameNotEmpty) {
  auto sources = detector_->getDisplaySources();

  for (const auto& source : sources) {
    EXPECT_FALSE(source.name.empty()) << "Display source name should not be empty";
  }
}

// Test 7: DisplaySourceAtomicActiveState
// Verify that display source is_active is accessible
TEST_F(DisplayDetectorDxgiTest, DisplaySourceAtomicActiveState) {
  auto sources = detector_->getDisplaySources();

  for (const auto& source : sources) {
    bool active = source.is_active;
    EXPECT_TRUE(active) << "Display source should be active initially";
  }
}

// Test 8: GetDisplaySourceByIdValid
// Verify that getDisplaySource() returns valid display for valid ID
TEST_F(DisplayDetectorDxgiTest, GetDisplaySourceByIdValid) {
  auto sources = detector_->getDisplaySources();

  if (!sources.empty()) {
    int valid_id = sources[0].id;
    auto source = detector_->getDisplaySource(valid_id);

    EXPECT_EQ(source.id, valid_id) << "Display source ID should match requested ID";
    EXPECT_FALSE(source.name.empty()) << "Display source name should not be empty";
    EXPECT_GT(source.resolution_width, 0) << "Display source width should be > 0";
  }
}

// Test 9: GetDisplaySourceByIdInvalid
// Verify that getDisplaySource() returns empty display for invalid ID
TEST_F(DisplayDetectorDxgiTest, GetDisplaySourceByIdInvalid) {
  auto invalid_id = 999;
  auto source = detector_->getDisplaySource(invalid_id);

  EXPECT_EQ(source.id, 0) << "Invalid ID should return display with ID 0";
  EXPECT_TRUE(source.name.empty()) << "Invalid ID should return display with empty name";
  EXPECT_EQ(source.resolution_width, 0) << "Invalid ID should return display with width 0";
  EXPECT_EQ(source.resolution_height, 0) << "Invalid ID should return display with height 0";
}

// Test 10: DisplaySourceCountMatchesGetSize
// Verify that getDisplaySourceCount() matches the size of getDisplaySources()
TEST_F(DisplayDetectorDxgiTest, DisplaySourceCountMatchesGetSize) {
  auto sources = detector_->getDisplaySources();
  int count = detector_->getDisplaySourceCount();

  EXPECT_EQ(count, static_cast<int>(sources.size()))
      << "Display source count should match vector size";
}

} // namespace screensdk
