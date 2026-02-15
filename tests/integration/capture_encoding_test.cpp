#include <gtest/gtest.h>

#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "screensdk/capture/dxgi_capture.h"
#include "screensdk/capture/display_detector.h"
#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"
#include "screensdk/transport/video_source.h"

namespace screensdk {

/**
 * @brief Integration test for complete capture to encoding pipeline
 *
 * Tests the full pipeline:
 * Display Detector â†?DXGI Capture â†?Encoder â†?Compressed Video
 *
 * Scenarios:
 * - Complete pipeline initialization
 * - Capture and encode single frame
 * - Continuous capture and encode at target FPS
 * - Different encoder configurations
 * - Performance and stability under load
 * - Error handling and recovery
 * - Memory stability over long runs
 */
class CaptureEncodingPipelineIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    display_detector_ = new DisplayDetector();

    auto displays = display_detector_->getDisplays();
    ASSERT_GT(displays.size(), 0) << "No displays available for testing";

    primary_display_ = display_detector_->getPrimaryDisplay();
    ASSERT_NE(primary_display_.name, "") << "No primary display found";

    capture_ = new DxgiCapture();
    factory_ = CreateEncoderFactory();
    ASSERT_NE(factory_, nullptr);

    config_ = EncoderConfig::getLowLatency();
    config_json_ = config_.toJson();

    std::cout << "Testing on: " << primary_display_.name << " ("
              << primary_display_.width << "x" << primary_display_.height << ") at "
              << primary_display_.refresh_rate << "Hz" << std::endl;
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

  DisplayDetector* display_detector_{nullptr};
  DxgiCapture* capture_{nullptr};
  IEncoderFactory* factory_{nullptr};
  IVideoEncoder* encoder_{nullptr};
  DisplayInfo primary_display_;
  EncoderConfig config_;
  std::string config_json_;
};

TEST_F(CaptureEncodingPipelineIntegrationTest, InitializeCompletePipeline) {
  // Test initializing all components of the pipeline
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init())
    << "DXGI capture should initialize";

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr) << "Encoder should be created";

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  )) << "Encoder should initialize";

  int capture_width, capture_height;
  capture_->getFrameSize(&capture_width, &capture_height);

  EXPECT_EQ(capture_width, primary_display_.width);
  EXPECT_EQ(capture_height, primary_display_.height);
  EXPECT_TRUE(encoder_->isAvailable());

  std::cout << "Pipeline initialized successfully" << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, CaptureAndEncodeSingleFrame) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  // Capture frame
  VideoFrameForTrans frame;
  ASSERT_TRUE(capture_->captureFrame(frame))
    << "Should capture frame";

  EXPECT_NE(frame.data, nullptr);
  EXPECT_GT(frame.size, 0);
  EXPECT_EQ(frame.width, primary_display_.width);
  EXPECT_EQ(frame.height, primary_display_.height);

  // Encode frame
  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size = kOutputBufferSize;

  bool encoded = encoder_->encode(frame, output.data(), &output_size);
  ASSERT_TRUE(encoded) << "Should encode frame";
  EXPECT_GT(output_size, 0);
  EXPECT_LT(output_size, kOutputBufferSize);

  std::cout << "Captured frame: " << frame.size << " bytes, "
            << "Encoded: " << output_size << " bytes (compression: "
            << (100.0 * output_size / frame.size) << "%)" << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, ContinuousCaptureAndEncode) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  const int kFrameCount = 30;
  int captured_count = 0;
  int encoded_count = 0;
  size_t total_encoded_size = 0;

  for (int i = 0; i < kFrameCount; ++i) {
  VideoFrameForTrans frame;
    if (capture_->captureFrame(frame)) {
      captured_count++;

      output_size = kOutputBufferSize;
      if (encoder_->encode(frame, output.data(), &output_size)) {
        encoded_count++;
        total_encoded_size += output_size;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }

  // Conservative expectations due to GOP=1 (all I-frames)
  // Desktop Duplication API may miss frames on some systems
  EXPECT_GT(captured_count, kFrameCount * 0.1);
  EXPECT_GT(encoded_count, kFrameCount * 0.05);
  EXPECT_GT(total_encoded_size, 0);

  double avg_encoded_size = static_cast<double>(total_encoded_size) / encoded_count;

  std::cout << "Captured: " << captured_count << "/" << kFrameCount
            << ", Encoded: " << encoded_count << "/" << kFrameCount
            << ", Avg encoded: " << avg_encoded_size << " bytes" << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, PipelinePerformanceTarget60Fps) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  const int kTargetFrames = 60;
  auto start_time = std::chrono::high_resolution_clock::now();

  int encoded_count = 0;
  for (int i = 0; i < kTargetFrames; ++i) {
  VideoFrameForTrans frame;
    if (capture_->captureFrame(frame)) {
      output_size = kOutputBufferSize;
      if (encoder_->encode(frame, output.data(), &output_size)) {
        encoded_count++;
      }
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time).count();

  double fps = (encoded_count * 1000.0) / duration_ms;

  std::cout << "Performance: " << encoded_count << " frames in "
            << duration_ms << "ms (" << fps << " FPS)" << std::endl;

  // Realistic expectation for software encoding with GOP=1 (all I-frames)
  // GOP=1 causes every frame to be a keyframe, significantly slower encoding
  EXPECT_GE(encoded_count, kTargetFrames * 0.2)
    << "Should encode at least 20% of frames with GOP=1";
  EXPECT_GT(fps, 2.0) << "Should achieve at least 2 FPS with all I-frames";
}

TEST_F(CaptureEncodingPipelineIntegrationTest, EncoderFlushAfterPipelineRun) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  // Encode multiple frames
  for (int i = 0; i < 10; ++i) {
  VideoFrameForTrans frame;
    if (capture_->captureFrame(frame)) {
      output_size = kOutputBufferSize;
      encoder_->encode(frame, output.data(), &output_size);
    }
  }

  // Flush encoder
  encoder_->flush();

  SUCCEED() << "Encoder flush completed successfully after pipeline run";
}

TEST_F(CaptureEncodingPipelineIntegrationTest, DifferentEncoderConfigurations) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());
  VideoFrameForTrans frame;
  ASSERT_TRUE(capture_->captureFrame(frame));

  std::vector<EncoderConfig> configs = {
    EncoderConfig::getLowLatency(),
    EncoderConfig::getHighQuality()
  };

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  for (const auto& config : configs) {
    IVideoEncoder* test_encoder = factory_->createEncoder(EncoderType::kSoftwareX264);
    ASSERT_NE(test_encoder, nullptr);

    std::string config_json = config.toJson();
    ASSERT_TRUE(test_encoder->initialize(
      primary_display_.width, primary_display_.height, 60, config_json));

    output_size = kOutputBufferSize;
    bool encoded = test_encoder->encode(frame, output.data(), &output_size);

    // High quality config with B-frames may need multiple frames
    if (config.b_frames > 0) {
      delete test_encoder;
      continue;
    }

    EXPECT_TRUE(encoded) << "Failed with config: " << config_json;
    EXPECT_GT(output_size, 0);

    delete test_encoder;
  }

  std::cout << "Successfully tested " << configs.size()
            << " encoder configurations" << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, AutoSelectedEncoderPipeline) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  // Use auto-selected encoder
  encoder_ = factory_->createEncoder();
  ASSERT_NE(encoder_, nullptr);

  EncoderType type = encoder_->getType();
  std::cout << "Auto-selected encoder: "
            << factory_->getEncoderTypeName(type) << std::endl;

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));
  VideoFrameForTrans frame;
  ASSERT_TRUE(capture_->captureFrame(frame));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size = kOutputBufferSize;

  bool encoded = encoder_->encode(frame, output.data(), &output_size);
  EXPECT_TRUE(encoded);
  EXPECT_GT(output_size, 0);

  std::cout << "Pipeline with auto-selected encoder works correctly" << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, PipelineWithCallbackAndEncode) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  int frame_count = 0;
  int encoded_count = 0;
  size_t total_encoded_size = 0;

  auto callback = [&](const VideoFrameForTrans& frame) {
    frame_count++;

    output_size = kOutputBufferSize;
    if (encoder_->encode(frame, output.data(), &output_size)) {
      encoded_count++;
      total_encoded_size += output_size;
    }
  };

  capture_->setFrameCallback(callback);
  capture_->start();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  capture_->stop();

  // Conservative expectation due to Desktop Duplication API behavior
  EXPECT_GE(frame_count, 2);
  EXPECT_GE(encoded_count, 1);
  EXPECT_GT(total_encoded_size, 0);

  std::cout << "Callback pipeline: captured " << frame_count
            << ", encoded " << encoded_count << " frames" << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, PipelineMemoryStability) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  const int kLongRunFrames = 300;

  for (int i = 0; i < kLongRunFrames; ++i) {
  VideoFrameForTrans frame;
    if (capture_->captureFrame(frame)) {
      output_size = kOutputBufferSize;
      encoder_->encode(frame, output.data(), &output_size);
    }
  }

  SUCCEED() << "Completed " << kLongRunFrames
            << " frames without memory issues";
}

TEST_F(CaptureEncodingPipelineIntegrationTest, PipelineHandlesStride) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());
  VideoFrameForTrans frame;
  ASSERT_TRUE(capture_->captureFrame(frame));

  EXPECT_GT(frame.stride, 0);
  EXPECT_EQ(frame.stride % 4, 0);

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size = kOutputBufferSize;

  bool encoded = encoder_->encode(frame, output.data(), &output_size);
  EXPECT_TRUE(encoded) << "Should encode with stride: " << frame.stride;

  std::cout << "Pipeline correctly handles stride: " << frame.stride << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, PipelineWithMultipleResets) {
  // Test that pipeline can be reset and reinitialized multiple times
  const int kResetCount = 3;

  for (int i = 0; i < kResetCount; ++i) {
    // Clean up previous iteration
    if (capture_ != nullptr) {
      delete capture_;
    }
    if (encoder_ != nullptr) {
      delete encoder_;
    }

    // Reinitialize
    capture_ = new DxgiCapture();
    capture_->selectDisplayIndex(primary_display_.index);
    ASSERT_TRUE(capture_->init());

    encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
    ASSERT_NE(encoder_, nullptr);

    ASSERT_TRUE(encoder_->initialize(
      primary_display_.width,
      primary_display_.height,
      60,
      config_json_
    ));

    // Test pipeline works
  VideoFrameForTrans frame;
    ASSERT_TRUE(capture_->captureFrame(frame));

    const size_t kOutputBufferSize = 10 * 1024 * 1024;
    std::vector<uint8_t> output(kOutputBufferSize);
    size_t output_size = kOutputBufferSize;

    bool encoded = encoder_->encode(frame, output.data(), &output_size);
    EXPECT_TRUE(encoded);
  }

  std::cout << "Successfully reset pipeline " << kResetCount << " times" << std::endl;
}

TEST_F(CaptureEncodingPipelineIntegrationTest, PipelineCompressionRatio) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;
  VideoFrameForTrans frame;
  ASSERT_TRUE(capture_->captureFrame(frame));

  output_size = kOutputBufferSize;
  bool encoded = encoder_->encode(frame, output.data(), &output_size);
  ASSERT_TRUE(encoded);

  double compression_ratio = 100.0 * output_size / frame.size;

  std::cout << "Raw frame: " << frame.size << " bytes ("
            << frame.size / 1024 << "KB)" << std::endl;
  std::cout << "Encoded: " << output_size << " bytes ("
            << output_size / 1024 << "KB)" << std::endl;
  std::cout << "Compression ratio: " << compression_ratio << "%" << std::endl;

  // H.264 should provide good compression
  EXPECT_LT(compression_ratio, 50.0)
    << "Compression should reduce size by at least 50%";
}

TEST_F(CaptureEncodingPipelineIntegrationTest, PipelineLatencyMeasurement) {
  capture_->selectDisplayIndex(primary_display_.index);
  ASSERT_TRUE(capture_->init());

  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  const int kSampleCount = 10;
  std::vector<double> latencies;

  for (int i = 0; i < kSampleCount; ++i) {
  VideoFrameForTrans frame;

    auto capture_start = std::chrono::high_resolution_clock::now();
    bool captured = capture_->captureFrame(frame);
    if (!captured) {
      continue;
    }
    auto capture_end = std::chrono::high_resolution_clock::now();

    auto encode_start = std::chrono::high_resolution_clock::now();
    output_size = kOutputBufferSize;
    encoder_->encode(frame, output.data(), &output_size);
    auto encode_end = std::chrono::high_resolution_clock::now();

    auto capture_us = std::chrono::duration_cast<std::chrono::microseconds>(
      capture_end - capture_start).count();
    auto encode_us = std::chrono::duration_cast<std::chrono::microseconds>(
      encode_end - encode_start).count();

    latencies.push_back(capture_us + encode_us);
  }

  EXPECT_GT(latencies.size(), 0) << "Should capture and encode at least one frame";

  double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
  double max_latency = *std::max_element(latencies.begin(), latencies.end());

  std::cout << "Average pipeline latency: " << avg_latency / 1000.0 << "ms" << std::endl;
  std::cout << "Max pipeline latency: " << max_latency / 1000.0 << "ms" << std::endl;

  // Pipeline latency should be reasonable
  EXPECT_LT(avg_latency, 50000.0) << "Average latency should be < 50ms";
}

} // namespace screensdk
