#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include "screensdk/capture/dxgi_capture.h"
#include "screensdk/capture/display_detector.h"

using namespace screensdk;

class DxgiCaptureTest : public ::testing::Test {
protected:
  void SetUp() override {
    detector_ = std::make_unique<DisplayDetector>();
    capture_ = std::make_unique<DxgiCapture>();
  }

  void TearDown() override {
    if (capture_ && capture_->isRunning()) {
      capture_->stop();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  std::unique_ptr<DisplayDetector> detector_;
  std::unique_ptr<DxgiCapture> capture_;
};

TEST_F(DxgiCaptureTest, InitializeSuccess) {
  auto displays = detector_->getDisplays();
  ASSERT_FALSE(displays.empty()) << "No displays available for testing";

  bool result = capture_->initialize(0);

  EXPECT_TRUE(result);
}

TEST_F(DxgiCaptureTest, InitializeInvalidDisplayIndex) {
  // Test with invalid display index
  bool result = capture_->initialize(9999);

  EXPECT_FALSE(result);
}

TEST_F(DxgiCaptureTest, InitializeDefaultDisplay) {
  // Test default initialization (display index 0)
  bool result = capture_->initialize();

  EXPECT_TRUE(result);
}

TEST_F(DxgiCaptureTest, GetFrameSizeAfterInitialize) {
  capture_->initialize(0);

  int width = 0;
  int height = 0;
  capture_->getFrameSize(&width, &height);

  EXPECT_GT(width, 0);
  EXPECT_GT(height, 0);

  // Validate dimensions are reasonable
  EXPECT_LE(width, 7680);   // Max 8K
  EXPECT_LE(height, 4320);
  EXPECT_GE(width, 640);     // Min 640x480
  EXPECT_GE(height, 480);
}

TEST_F(DxgiCaptureTest, GetFrameSizeNullPtrs) {
  capture_->initialize(0);

  // Should handle null pointers gracefully
  capture_->getFrameSize(nullptr, nullptr);

  SUCCEED();
}

TEST_F(DxgiCaptureTest, CaptureFrameValid) {
  capture_->initialize(0);

  VideoFrame frame;
  bool result = capture_->captureFrame(frame);

  EXPECT_TRUE(result);
  EXPECT_NE(frame.data, nullptr);
  EXPECT_GT(frame.size, 0);
  EXPECT_GT(frame.width, 0);
  EXPECT_GT(frame.height, 0);
}

TEST_F(DxgiCaptureTest, CaptureFrameDimensionsMatchDisplay) {
  capture_->initialize(0);

  int capture_width = 0;
  int capture_height = 0;
  capture_->getFrameSize(&capture_width, &capture_height);

  VideoFrame frame;
  capture_->captureFrame(frame);

  EXPECT_EQ(frame.width, capture_width);
  EXPECT_EQ(frame.height, capture_height);
}

TEST_F(DxgiCaptureTest, CaptureFrameStrideValid) {
  capture_->initialize(0);

  VideoFrame frame;
  capture_->captureFrame(frame);

  EXPECT_GT(frame.stride, 0);
  EXPECT_EQ(frame.size, frame.height * frame.stride);

  // Stride should be multiple of 4 (BGRA alignment)
  EXPECT_EQ(frame.stride % 4, 0);
}

TEST_F(DxgiCaptureTest, CaptureFrameTimestampValid) {
  capture_->initialize(0);

  VideoFrame frame;
  capture_->captureFrame(frame);

  uint64_t before = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();

  VideoFrame frame2;
  capture_->captureFrame(frame2);

  uint64_t after = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();

  EXPECT_GE(frame.timestamp_ms, before - 1000);
  EXPECT_LE(frame.timestamp_ms, after + 1000);
}

TEST_F(DxgiCaptureTest, StartStopCapture) {
  capture_->initialize(0);

  EXPECT_FALSE(capture_->isRunning());

  capture_->start();
  EXPECT_TRUE(capture_->isRunning());

  capture_->stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_FALSE(capture_->isRunning());
}

TEST_F(DxgiCaptureTest, StartWhenAlreadyRunning) {
  capture_->initialize(0);
  capture_->start();

  // Calling start again should not crash
  capture_->start();

  EXPECT_TRUE(capture_->isRunning());

  capture_->stop();
}

TEST_F(DxgiCaptureTest, StopWhenNotRunning) {
  capture_->initialize(0);

  // Calling stop when not running should not crash
  capture_->stop();

  EXPECT_FALSE(capture_->isRunning());
}

TEST_F(DxgiCaptureTest, FrameCallbackInvoked) {
  capture_->initialize(0);

  std::atomic<int> callback_count{0};
  VideoFrame last_frame;

  capture_->setFrameCallback([&callback_count, &last_frame](const VideoFrame& frame) {
    callback_count++;
    last_frame = frame;
  });

  capture_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  capture_->stop();

  EXPECT_GT(callback_count, 0);
  EXPECT_GT(last_frame.width, 0);
  EXPECT_GT(last_frame.height, 0);
}

TEST_F(DxgiCaptureTest, FrameCallbackNull) {
  capture_->initialize(0);

  // Setting null callback should not crash
  capture_->setFrameCallback(nullptr);

  capture_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  capture_->stop();

  SUCCEED();
}

TEST_F(DxgiCaptureTest, TargetFpsControl) {
  capture_->initialize(0);

  capture_->setTargetFps(30);
  EXPECT_EQ(capture_->getFps(), 30);

  capture_->setTargetFps(60);
  EXPECT_EQ(capture_->getFps(), 60);

  capture_->setTargetFps(120);
  EXPECT_EQ(capture_->getFps(), 120);
}

TEST_F(DxgiCaptureTest, FpsRateMeasurement) {
  capture_->initialize(0);

  std::atomic<int> frame_count{0};

  capture_->setFrameCallback([&frame_count](const VideoFrame&) {
    frame_count++;
  });

  capture_->setTargetFps(60);
  capture_->start();

  // Run for longer to get more stable FPS measurement
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  capture_->stop();

  // Desktop Duplication API only returns frames when screen content changes
  // In a test environment with minimal screen changes, frame count will be lower
  // This test verifies the capture loop is running, not exact FPS
  EXPECT_GT(frame_count, 0) << "No frames captured - capture loop may not be running";

  // Verify we got at least a few frames (not strict FPS test)
  // Real-world FPS will be determined by actual screen activity
  if (frame_count > 0) {
    // If frames were captured, verify reasonable rate (not 1 frame per second)
    // Lower threshold for test environment with minimal screen activity
    EXPECT_GT(frame_count, 2) << "Frame rate too low: " << frame_count << " frames/sec";
  }
}

TEST_F(DxgiCaptureTest, IsRunningState) {
  capture_->initialize(0);

  EXPECT_FALSE(capture_->isRunning());

  capture_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(capture_->isRunning());

  capture_->stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(capture_->isRunning());
}

TEST_F(DxgiCaptureTest, InitializeMultipleDisplays) {
  auto displays = detector_->getDisplays();

  // Test initializing different displays with separate instances
  for (const auto& display : displays) {
    auto test_capture = std::make_unique<DxgiCapture>();

    bool result = test_capture->initialize(display.index);

    // Note: Only one display can be active at a time
    // This test creates and destroys each instance sequentially
    if (display.is_primary) {
      EXPECT_TRUE(result) << "Failed to initialize primary display " << display.index;
    }

    test_capture.reset();  // Explicitly destroy to release DXGI resources
  }
}

TEST_F(DxgiCaptureTest, MultipleCaptureInstancesOnSameDisplay) {
  // Desktop Duplication API limitation: only one instance per display
  auto capture2 = std::make_unique<DxgiCapture>();
  auto capture3 = std::make_unique<DxgiCapture>();

  // First instance (capture_ from SetUp) should succeed
  // Note: capture_ may already be initialized from previous tests
  if (!capture_->isRunning()) {
    EXPECT_TRUE(capture_->initialize(0));
  }

  // Second and third instances should fail (DXGI limitation)
  EXPECT_FALSE(capture2->initialize(0));
  EXPECT_FALSE(capture3->initialize(0));
}

TEST_F(DxgiCaptureTest, ReinitializeAfterDestruction) {
  capture_->initialize(0);
  capture_->start();
  capture_->stop();

  // Destroy first instance (this releases DXGI resources)
  capture_.reset();

  // Create a new instance - should succeed after first is destroyed
  auto capture2 = std::make_unique<DxgiCapture>();
  EXPECT_TRUE(capture2->initialize(0));

  // Restore capture_ to avoid crash in TearDown
  capture_ = std::move(capture2);
}

TEST_F(DxgiCaptureTest, StopTokenStopsLoop) {
  capture_->initialize(0);

  std::atomic<bool> loop_running{false};

  capture_->setFrameCallback([&loop_running](const VideoFrame&) {
    loop_running = true;
  });

  capture_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  capture_->stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Loop should have been running before stop
  EXPECT_TRUE(loop_running);
  EXPECT_FALSE(capture_->isRunning());
}

TEST_F(DxgiCaptureTest, CaptureFrameSizeCalculation) {
  capture_->initialize(0);

  VideoFrame frame;
  capture_->captureFrame(frame);

  // Expected size: height * stride (BGRA format with alignment)
  size_t expected_size = frame.height * frame.stride;
  EXPECT_EQ(frame.size, expected_size);

  // Stride should be at least width * 4 bytes (BGRA)
  EXPECT_GE(frame.stride, frame.width * 4);
}
