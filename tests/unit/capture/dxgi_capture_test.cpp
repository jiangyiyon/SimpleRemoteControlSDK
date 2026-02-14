/**
 * @file dxgi_capture_test.cpp
 * @brief Unit tests for DxgiCapture 60fps capture loop
 *
 * T037: Implement DXGI screen capture loop at 60fps
 * TDD Approach: Write tests first, then implement
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "screensdk/capture/dxgi_capture.h"

using namespace screensdk;

// ============================================================================
// DxgiCapture Tests
// ============================================================================

class DxgiCaptureTest : public ::testing::Test {
protected:
  void SetUp() override {
    capture_ = std::make_unique<DxgiCapture>();
  }

  void TearDown() override {
    if (capture_) {
      capture_->stop();
    }
  }

  std::unique_ptr<DxgiCapture> capture_;
};

TEST_F(DxgiCaptureTest, InitializeWithDefaultDisplay) {
  bool initialized = capture_->initialize(0);

  if (initialized) {
    int width, height;
    capture_->getFrameSize(&width, &height);
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
  }
}

TEST_F(DxgiCaptureTest, InitializeWithInvalidDisplay) {
  bool initialized = capture_->initialize(999);

  EXPECT_FALSE(initialized);
}

TEST_F(DxgiCaptureTest, StartWithoutInitialize) {
  capture_->start();
  capture_->stop();
}

TEST_F(DxgiCaptureTest, SetAndGetTargetFps) {
  capture_->setTargetFps(30);
  EXPECT_EQ(capture_->getFps(), 30);

  capture_->setTargetFps(60);
  EXPECT_EQ(capture_->getFps(), 60);
}

TEST_F(DxgiCaptureTest, FrameCallback) {
  bool initialized = capture_->initialize(0);
  if (!initialized) {
    GTEST_SKIP() << "Display initialization failed";
  }

  bool callback_called = false;
  int frame_count = 0;

  capture_->setFrameCallback([&](const VideoFrame& frame) {
    callback_called = true;
    frame_count++;
    EXPECT_NE(frame.data, nullptr);
    EXPECT_GT(frame.width, 0);
    EXPECT_GT(frame.height, 0);
    EXPECT_GT(frame.size, 0);
  });

  capture_->start();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  capture_->stop();

  EXPECT_TRUE(callback_called);
  EXPECT_GT(frame_count, 0);
}

TEST_F(DxgiCaptureTest, StopAndRestartCapture) {
  bool initialized = capture_->initialize(0);
  if (!initialized) {
    GTEST_SKIP() << "Display initialization failed";
  }

  int start_count = 0;
  int restart_count = 0;

  capture_->setFrameCallback([&](const VideoFrame& frame) {
    if (start_count < 50) {
      start_count++;
    } else {
      restart_count++;
    }
  });

  capture_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  capture_->stop();

  capture_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  capture_->stop();

  EXPECT_GT(start_count, 0);
  EXPECT_GT(restart_count, 0);
}

TEST_F(DxgiCaptureTest, MultipleStops) {
  bool initialized = capture_->initialize(0);
  if (!initialized) {
    GTEST_SKIP() << "Display initialization failed";
  }

  capture_->start();
  capture_->stop();
  capture_->stop();
  capture_->stop();
}
