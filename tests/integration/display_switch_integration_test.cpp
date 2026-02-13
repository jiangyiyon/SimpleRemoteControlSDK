#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

#include "screensdk/capture/dxgi_capture.h"
#include "screensdk/capture/display_detector.h"
#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"

namespace screensdk {

/**
 * @brief Integration test for display switching with capture and encoding
 *
 * Tests the complete pipeline for multi-display scenarios:
 * DisplayDetector → DxgiCapture → Encoder (switch handling)
 *
 * Key Integration Points:
 * - DisplayDetector enumerates displays
 * - DxgiCapture reinitializes on display switch
 * - Encoder reinitializes with new resolution
 * - Pipeline maintains consistency across switches
 *
 * Scenarios:
 * - Complete pipeline initialization on multiple displays
 * - Display switch with capture and encoding
 * - Continuous capture across display switch
 * - Encoder reconfiguration after display switch
 * - Performance impact of display switch
 * - Multiple rapid display switches
 */
class DisplaySwitchIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    display_detector_ = new DisplayDetector();

    auto displays = display_detector_->getDisplays();
    ASSERT_GT(displays.size(), 0) << "No displays available for testing";

    displays_ = displays;

    std::cout << "Found " << displays_.size() << " display(s):" << std::endl;
    for (size_t i = 0; i < displays_.size(); ++i) {
      std::cout << "  Display " << displays_[i].index << ": "
                << displays_[i].name << " ("
                << displays_[i].width << "x" << displays_[i].height << ")"
                << (displays_[i].is_primary ? " [Primary]" : "") << std::endl;
    }

    capture_ = new DxgiCapture();
    factory_ = CreateEncoderFactory();
    ASSERT_NE(factory_, nullptr);

    config_ = EncoderConfig::getLowLatency();
    config_json_ = config_.toJson();
  }

  void TearDown() override {
    if (capture_ != nullptr) {
      if (capture_->isRunning()) {
        capture_->stop();
      }
      delete capture_;
      capture_ = nullptr;
    }

    if (encoder_ != nullptr) {
      encoder_->flush();
      delete encoder_;
      encoder_ = nullptr;
    }

    if (factory_ != nullptr) {
      DestroyEncoderFactory(factory_);
      factory_ = nullptr;
    }

    if (display_detector_ != nullptr) {
      delete display_detector_;
      display_detector_ = nullptr;
    }
  }

  /**
   * @brief Initialize pipeline on specified display
   */
  void initializePipeline(int display_index) {
    ASSERT_TRUE(capture_->initialize(display_index))
      << "Capture should initialize on display " << display_index;

    DisplayInfo display = display_detector_->getDisplay(display_index);

    encoder_ = factory_->createEncoder();
    ASSERT_NE(encoder_, nullptr) << "Encoder should be created";

    ASSERT_TRUE(encoder_->initialize(
      display.width,
      display.height,
      60,
      config_json_
    )) << "Encoder should initialize";
  }

  /**
   * @brief Capture and encode a frame
   */
  bool captureAndEncodeFrame(size_t* encoded_size = nullptr) {
    VideoFrame frame;
    if (!capture_->captureFrame(frame)) {
      return false;
    }

    const size_t kOutputBufferSize = 10 * 1024 * 1024;
    std::vector<uint8_t> output(kOutputBufferSize);
    size_t output_size = kOutputBufferSize;

    bool success = encoder_->encode(frame, output.data(), &output_size);

    if (encoded_size != nullptr && success) {
      *encoded_size = output_size;
    }

    return success;
  }

  DisplayDetector* display_detector_{nullptr};
  DxgiCapture* capture_{nullptr};
  IEncoderFactory* factory_{nullptr};
  IVideoEncoder* encoder_{nullptr};

  std::vector<DisplayInfo> displays_;
  EncoderConfig config_;
  std::string config_json_;
};

TEST_F(DisplaySwitchIntegrationTest, InitializePipelineOnPrimaryDisplay) {
  // Find primary display
  DisplayInfo primary;
  for (const auto& display : displays_) {
    if (display.is_primary) {
      primary = display;
      break;
    }
  }

  ASSERT_NE(primary.index, -1) << "Primary display should be found";

  // Initialize complete pipeline
  initializePipeline(primary.index);

  int width, height;
  capture_->getFrameSize(&width, &height);

  EXPECT_EQ(width, primary.width);
  EXPECT_EQ(height, primary.height);
  EXPECT_TRUE(encoder_->isAvailable());

  std::cout << "Pipeline initialized on primary: "
            << primary.name << " (" << width << "x" << height << ")" << std::endl;
}

TEST_F(DisplaySwitchIntegrationTest, CaptureAndEncodeOnSingleDisplay) {
  if (displays_.empty()) {
    GTEST_SKIP() << "No displays available";
  }

  DisplayInfo primary = display_detector_->getPrimaryDisplay();
  initializePipeline(primary.index);

  // Capture and encode multiple frames
  const int kFrameCount = 10;
  int success_count = 0;

  for (int i = 0; i < kFrameCount; ++i) {
    size_t encoded_size;
    if (captureAndEncodeFrame(&encoded_size)) {
      success_count++;
      EXPECT_GT(encoded_size, 0);
    }
  }

  EXPECT_GE(success_count, kFrameCount * 0.3)
    << "Should encode most frames: " << success_count << "/" << kFrameCount;

  std::cout << "Encoded " << success_count << "/" << kFrameCount << " frames"
            << std::endl;
}

TEST_F(DisplaySwitchIntegrationTest, SwitchDisplayAndReinitializePipeline) {
  if (displays_.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays, found: " << displays_.size();
  }

  // Initialize on first display
  int from_index = displays_[0].index;
  int to_index = displays_[1].index;

  initializePipeline(from_index);

  // Capture a frame
  VideoFrame frame1;
  ASSERT_TRUE(capture_->captureFrame(frame1));
  EXPECT_EQ(frame1.width, displays_[0].width);
  EXPECT_EQ(frame1.height, displays_[0].height);

  // Measure switch time
  auto start = std::chrono::high_resolution_clock::now();

  // Switch display (reinitialize pipeline)
  delete capture_;
  capture_ = new DxgiCapture();

  delete encoder_;
  encoder_ = nullptr;

  initializePipeline(to_index);

  auto end = std::chrono::high_resolution_clock::now();
  auto switch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    end - start).count();

  // Capture frame from new display
  VideoFrame frame2;
  ASSERT_TRUE(capture_->captureFrame(frame2));
  EXPECT_EQ(frame2.width, displays_[1].width);
  EXPECT_EQ(frame2.height, displays_[1].height);

  std::cout << "Display " << from_index << " → " << to_index
            << " switch time: " << switch_ms << "ms" << std::endl;

  // Switch should be reasonably fast
  EXPECT_LT(switch_ms, 500) << "Switch should complete in < 500ms";
}

TEST_F(DisplaySwitchIntegrationTest, EncodeFramesFromDifferentDisplays) {
  if (displays_.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
  }

  // Test encoding from each display
  for (const auto& display : displays_) {
    // Clean up previous iteration
    if (capture_ != nullptr) {
      delete capture_;
    }
    if (encoder_ != nullptr) {
      delete encoder_;
      encoder_ = nullptr;
    }

    // Initialize on this display
    capture_ = new DxgiCapture();
    initializePipeline(display.index);

    // Capture and encode frame
    size_t encoded_size;
    ASSERT_TRUE(captureAndEncodeFrame(&encoded_size));

    std::cout << "Display " << display.index << " (" << display.name
              << "): encoded " << encoded_size << " bytes" << std::endl;

    EXPECT_GT(encoded_size, 0);
  }
}

TEST_F(DisplaySwitchIntegrationTest, ContinuousCaptureAcrossDisplaySwitch) {
  if (displays_.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
  }

  int from_index = displays_[0].index;
  int to_index = displays_[1].index;

  // Start continuous capture on first display
  initializePipeline(from_index);

  std::atomic<int> frame_count1{0};
  std::atomic<int> frame_count2{0};
  std::atomic<bool> switched{false};

  auto callback = [&](const VideoFrame& frame) {
    if (!switched.load()) {
      frame_count1++;
      if (frame_count1 >= 5) {
        // Switch display after 5 frames
        switched = true;
      }
    } else {
      frame_count2++;
    }
  };

  capture_->setFrameCallback(callback);
  capture_->start();

  // Wait for first frames
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Switch display
  capture_->stop();

  delete capture_;
  capture_ = new DxgiCapture();

  delete encoder_;
  encoder_ = nullptr;

  initializePipeline(to_index);

  // Continue capture on new display
  capture_->setFrameCallback(callback);
  capture_->start();

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  capture_->stop();

  std::cout << "Frames before switch: " << frame_count1.load()
            << ", after switch: " << frame_count2.load() << std::endl;

  EXPECT_GT(frame_count1.load(), 0) << "Should capture frames before switch";
  EXPECT_GT(frame_count2.load(), 0) << "Should capture frames after switch";
}

TEST_F(DisplaySwitchIntegrationTest, MultipleRapidDisplaySwitches) {
  if (displays_.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
  }

  const int kSwitchCount = 5;
  int from_index = displays_[0].index;
  int to_index = displays_[1].index;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < kSwitchCount; ++i) {
    // Clean up
    delete capture_;
    delete encoder_;
    encoder_ = nullptr;

    // Initialize and capture
    capture_ = new DxgiCapture();
    initializePipeline(from_index);

    size_t encoded_size1;
    ASSERT_TRUE(captureAndEncodeFrame(&encoded_size1));

    // Switch
    delete capture_;
    capture_ = new DxgiCapture();

    delete encoder_;
    encoder_ = nullptr;

    initializePipeline(to_index);

    size_t encoded_size2;
    ASSERT_TRUE(captureAndEncodeFrame(&encoded_size2));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time).count();

  double avg_ms = static_cast<double>(total_ms) / (kSwitchCount * 2);

  std::cout << kSwitchCount << " switches (" << kSwitchCount * 2
            << " initializations) in " << total_ms << "ms, avg: "
            << avg_ms << "ms per init" << std::endl;

  // Average initialization should be reasonable
  EXPECT_LT(avg_ms, 200) << "Average init time should be < 200ms";
}

TEST_F(DisplaySwitchIntegrationTest, PipelineHandlesResolutionChange) {
  if (displays_.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
  }

  // Find displays with different resolutions if possible
  int idx1 = -1, idx2 = -1;

  for (size_t i = 0; i < displays_.size(); ++i) {
    for (size_t j = i + 1; j < displays_.size(); ++j) {
      if (displays_[i].width != displays_[j].width ||
          displays_[i].height != displays_[j].height) {
        idx1 = displays_[i].index;
        idx2 = displays_[j].index;
        break;
      }
    }
    if (idx1 != -1) break;
  }

  if (idx1 == -1) {
    GTEST_SKIP() << "No displays with different resolutions found";
  }

  // Initialize on first display
  initializePipeline(idx1);

  int width1, height1;
  capture_->getFrameSize(&width1, &height1);

  size_t encoded_size1;
  ASSERT_TRUE(captureAndEncodeFrame(&encoded_size1));

  // Switch to different resolution display
  delete capture_;
  capture_ = new DxgiCapture();

  delete encoder_;
  encoder_ = nullptr;

  initializePipeline(idx2);

  int width2, height2;
  capture_->getFrameSize(&width2, &height2);

  size_t encoded_size2;
  ASSERT_TRUE(captureAndEncodeFrame(&encoded_size2));

  std::cout << "Resolution change: " << width1 << "x" << height1
            << " → " << width2 << "x" << height2 << std::endl;

  // Verify resolution changed
  EXPECT_NE(width1, width2) << "Width should be different";
  EXPECT_NE(height1, height2) << "Height should be different";
}

TEST_F(DisplaySwitchIntegrationTest, PipelineStabilityAfterMultipleSwitches) {
  if (displays_.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
  }

  int from_index = displays_[0].index;
  int to_index = displays_[1].index;

  // Perform multiple switches and encode frames
  const int kIterations = 3;
  std::vector<size_t> encoded_sizes;

  for (int i = 0; i < kIterations; ++i) {
    // Clean up
    delete capture_;
    delete encoder_;
    encoder_ = nullptr;

    // Initialize on display 1
    capture_ = new DxgiCapture();
    initializePipeline(from_index);

    size_t size1;
    ASSERT_TRUE(captureAndEncodeFrame(&size1));
    encoded_sizes.push_back(size1);

    // Switch to display 2
    delete capture_;
    capture_ = new DxgiCapture();

    delete encoder_;
    encoder_ = nullptr;

    initializePipeline(to_index);

    size_t size2;
    ASSERT_TRUE(captureAndEncodeFrame(&size2));
    encoded_sizes.push_back(size2);
  }

  std::cout << "Successfully encoded " << encoded_sizes.size()
            << " frames across " << kIterations << " switches" << std::endl;

  // All frames should be encoded successfully
  EXPECT_EQ(encoded_sizes.size(), kIterations * 2);

  for (size_t i = 0; i < encoded_sizes.size(); ++i) {
    EXPECT_GT(encoded_sizes[i], 0);
  }
}

TEST_F(DisplaySwitchIntegrationTest, DisplaySwitchLatencyMeasurement) {
  if (displays_.size() < 2) {
    GTEST_SKIP() << "Need at least 2 displays";
  }

  const int kSampleCount = 5;
  std::vector<double> latencies;

  int from_index = displays_[0].index;
  int to_index = displays_[1].index;

  for (int i = 0; i < kSampleCount; ++i) {
    // Clean up
    delete capture_;
    delete encoder_;
    encoder_ = nullptr;

    // Initialize on display 1
    capture_ = new DxgiCapture();
    initializePipeline(from_index);

    VideoFrame frame1;
    ASSERT_TRUE(capture_->captureFrame(frame1));

    size_t size1;
    ASSERT_TRUE(captureAndEncodeFrame(&size1));

    // Measure switch time
    auto start = std::chrono::high_resolution_clock::now();

    // Switch
    delete capture_;
    capture_ = new DxgiCapture();

    delete encoder_;
    encoder_ = nullptr;

    initializePipeline(to_index);

    VideoFrame frame2;
    ASSERT_TRUE(capture_->captureFrame(frame2));

    size_t size2;
    ASSERT_TRUE(captureAndEncodeFrame(&size2));

    auto end = std::chrono::high_resolution_clock::now();

    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
      end - start).count();
    latencies.push_back(latency_us);
  }

  // Calculate statistics
  double avg_us = 0;
  for (auto lat : latencies) {
    avg_us += lat;
  }
  avg_us /= latencies.size();

  double max_us = *std::max_element(latencies.begin(), latencies.end());

  std::cout << "Display switch latency - Avg: " << (avg_us / 1000.0)
            << "ms, Max: " << (max_us / 1000.0) << "ms" << std::endl;

  // Switch latency should be reasonable
  EXPECT_LT(avg_us, 300000.0) << "Average switch should be < 300ms";
  EXPECT_LT(max_us, 500000.0) << "Max switch should be < 500ms";
}

} // namespace screensdk
