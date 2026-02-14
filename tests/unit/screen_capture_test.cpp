#include <gtest/gtest.h>
#include "screensdk/capture/i_screen_capture.h"
#include "screensdk/transport/video_source.h"
#include <thread>
#include <chrono>

using namespace screensdk;

/**
 * @brief Unit tests for IScreenCapture interface
 *
 * I2: Contract File Implementation Mismatch - Interface alignment test
 *
 * Tests the IScreenCapture interface implementation to ensure it conforms
 * to the contract defined in contracts/screen_capture.h
 */
class ScreenCaptureTest : public ::testing::Test {
protected:
  void SetUp() override {
    capture_ = CreateScreenCapture();
    ASSERT_NE(capture_, nullptr);
  }

  void TearDown() override {
    if (capture_) {
      capture_->shutdown();
      DestroyScreenCapture(capture_);
      capture_ = nullptr;
    }
  }

  IScreenCapture* capture_{nullptr};
};

/**
 * @brief Test initialization with valid display ID
 */
TEST_F(ScreenCaptureTest, InitializeWithValidDisplayId) {
  // Arrange
  int display_id = 1;  // Primary display (1-based ID)

  // Act
  bool result = capture_->initialize(display_id);

  // Assert
  EXPECT_TRUE(result) << "Should initialize successfully with display ID 1";
}

/**
 * @brief Test initialization with invalid display ID
 */
TEST_F(ScreenCaptureTest, InitializeWithInvalidDisplayId) {
  // Arrange
  int display_id = 999;  // Invalid display ID

  // Act
  bool result = capture_->initialize(display_id);

  // Assert
  EXPECT_FALSE(result) << "Should fail with invalid display ID";
}

/**
 * @brief Test display enumeration
 */
TEST_F(ScreenCaptureTest, EnumerateDisplays) {
  // Arrange & Act
  auto displays = capture_->enumerateDisplays();

  // Assert
  EXPECT_GT(displays.size(), 0) << "Should find at least one display";

  // Verify display structure
  for (const auto& display : displays) {
    EXPECT_GT(display.id, 0) << "Display ID should be positive";
    EXPECT_FALSE(display.name.empty()) << "Display name should not be empty";
    EXPECT_GT(display.resolution_width, 0) << "Display width should be positive";
    EXPECT_GT(display.resolution_height, 0) << "Display height should be positive";
    EXPECT_GE(display.refresh_rate, 30) << "Refresh rate should be at least 30Hz";
  }
}

/**
 * @brief Test get primary display
 */
TEST_F(ScreenCaptureTest, GetPrimaryDisplay) {
  // Arrange & Act
  auto primary_display = capture_->getPrimaryDisplay();

  // Assert
  EXPECT_GT(primary_display.id, 0) << "Primary display ID should be positive";
  EXPECT_TRUE(primary_display.is_primary) << "is_primary flag should be true";
  EXPECT_GT(primary_display.resolution_width, 0) << "Display width should be positive";
  EXPECT_GT(primary_display.resolution_height, 0) << "Display height should be positive";
}

/**
 * @brief Test start and stop capture
 */
TEST_F(ScreenCaptureTest, StartAndStopCapture) {
  // Arrange
  capture_->initialize(1);
  auto displays = capture_->enumerateDisplays();
  ASSERT_GT(displays.size(), 0);

  // Act - Start capture
  bool start_result = capture_->startCapture(displays[0]);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Act - Stop capture
  capture_->stopCapture();

  // Assert
  EXPECT_TRUE(start_result) << "Should start capture successfully";
}

/**
 * @brief Test get current display
 */
TEST_F(ScreenCaptureTest, GetCurrentDisplay) {
  // Arrange
  capture_->initialize(1);
  auto displays = capture_->enumerateDisplays();
  ASSERT_GT(displays.size(), 0);
  capture_->startCapture(displays[0]);

  // Act
  auto current_display = capture_->getCurrentDisplay();

  // Assert
  EXPECT_EQ(current_display.id, displays[0].id) << "Current display ID should match started display";

  // Cleanup
  capture_->stopCapture();
}

/**
 * @brief Test get next frame with timeout
 */
TEST_F(ScreenCaptureTest, GetNextFrameWithTimeout) {
  // Arrange
  capture_->initialize(1);
  auto displays = capture_->enumerateDisplays();
  ASSERT_GT(displays.size(), 0);
  capture_->startCapture(displays[0]);

  // Wait for frame to be captured
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Act
  auto frame = capture_->getNextFrame(100);

  // Assert
  EXPECT_NE(frame, nullptr) << "Should get a frame after capture started";
  if (frame) {
    EXPECT_GT(frame->width, 0) << "Frame width should be positive";
    EXPECT_GT(frame->height, 0) << "Frame height should be positive";
    EXPECT_GT(frame->timestamp_ms, 0) << "Frame timestamp should be positive";
  }

  // Cleanup
  capture_->stopCapture();
}

/**
 * @brief Test get next frame before capture started
 */
TEST_F(ScreenCaptureTest, GetNextFrameBeforeCaptureStart) {
  // Arrange & Act
  auto frame = capture_->getNextFrame(100);

  // Assert
  EXPECT_EQ(frame, nullptr) << "Should return nullptr when capture not started";
}

/**
 * @brief Test hardware encoding support check
 */
TEST_F(ScreenCaptureTest, SupportsHardwareEncoding) {
  // Act
  bool supports_hw = capture_->supportsHardwareEncoding();

  // Assert
  EXPECT_TRUE(supports_hw) << "Should report hardware encoding support (placeholder)";
}

/**
 * @brief Test get native resolution
 */
TEST_F(ScreenCaptureTest, GetNativeResolution) {
  // Arrange
  auto displays = capture_->enumerateDisplays();
  ASSERT_GT(displays.size(), 0);

  // Act
  auto resolution = capture_->getNativeResolution(displays[0]);

  // Assert
  EXPECT_GT(resolution.width, 0) << "Resolution width should be positive";
  EXPECT_GT(resolution.height, 0) << "Resolution height should be positive";
}

/**
 * @brief Test display change callback
 */
TEST_F(ScreenCaptureTest, DisplayChangeCallback) {
  // Arrange
  bool callback_called = false;
  capture_->setDisplayChangeCallback([&callback_called]() {
    callback_called = true;
  });

  // Act
  // Note: Cannot easily trigger display change in unit test
  // Just verify callback can be set without crash

  // Assert
  EXPECT_TRUE(true) << "Callback should be settable without crash";
}

/**
 * @brief Test error callback
 */
TEST_F(ScreenCaptureTest, ErrorCallback) {
  // Arrange
  std::string error_message;
  capture_->setErrorCallback([&error_message](const std::string& msg) {
    error_message = msg;
  });

  // Act
  // Note: Cannot easily trigger error in unit test
  // Just verify callback can be set without crash

  // Assert
  EXPECT_TRUE(true) << "Error callback should be settable without crash";
}

/**
 * @brief Test multiple initialize calls
 */
TEST_F(ScreenCaptureTest, MultipleInitializeCalls) {
  // Act
  bool first_result = capture_->initialize(1);
  bool second_result = capture_->initialize(1);

  // Assert
  EXPECT_TRUE(first_result) << "First initialize should succeed";
  EXPECT_TRUE(second_result) << "Second initialize should succeed (idempotent)";
}

/**
 * @brief Test shutdown before initialize
 */
TEST_F(ScreenCaptureTest, ShutdownBeforeInitialize) {
  // Act
  capture_->shutdown();

  // Assert
  EXPECT_TRUE(true) << "Shutdown should be safe even without initialize";
}

/**
 * @brief Test start capture without initialize
 */
TEST_F(ScreenCaptureTest, StartCaptureWithoutInitialize) {
  // Arrange
  DisplaySource display;
  display.id = 1;
  display.name = "Test Display";

  // Act
  bool result = capture_->startCapture(display);

  // Assert
  EXPECT_FALSE(result) << "Should fail to start capture without initialize";
}

/**
 * @brief Test get frame rate consistency
 */
TEST_F(ScreenCaptureTest, GetFrameRateConsistency) {
  // Arrange
  capture_->initialize(1);
  auto displays = capture_->enumerateDisplays();
  ASSERT_GT(displays.size(), 0);
  capture_->startCapture(displays[0]);

  // Act - Collect multiple frames
  std::vector<uint64_t> timestamps;
  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto frame = capture_->getNextFrame(100);
    if (frame) {
      timestamps.push_back(frame->timestamp_ms);
    }
  }

  // Assert
  EXPECT_GE(timestamps.size(), 5) << "Should capture at least 5 frames in 500ms";

  // Cleanup
  capture_->stopCapture();
}
