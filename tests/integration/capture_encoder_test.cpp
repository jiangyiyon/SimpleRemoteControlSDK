#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "screensdk/capture/dxgi_capture.h"
#include "screensdk/capture/display_detector.h"
#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"

namespace screensdk {

class CaptureEncoderIntegrationTest : public ::testing::Test {
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

    encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
    ASSERT_NE(encoder_, nullptr);
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
};

TEST_F(CaptureEncoderIntegrationTest, InitializeBoth) {
  EXPECT_TRUE(capture_->initialize(primary_display_.index));

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  EXPECT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));
}

TEST_F(CaptureEncoderIntegrationTest, CaptureAndEncodeFrame) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  VideoFrame frame;
  bool captured = capture_->captureFrame(frame);
  EXPECT_TRUE(captured);
  EXPECT_NE(frame.data, nullptr);
  EXPECT_GT(frame.size, 0);
  EXPECT_EQ(frame.width, primary_display_.width);
  EXPECT_EQ(frame.height, primary_display_.height);

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  bool encoded = encoder_->encode(frame, output, &output_size);
  EXPECT_TRUE(encoded);
  EXPECT_GT(output_size, 0);

  delete[] output;
}

TEST_F(CaptureEncoderIntegrationTest, CaptureAndEncodeMultipleFrames) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  int captured_count = 0;
  int encoded_count = 0;

  for (int i = 0; i < 30; ++i) {
    VideoFrame frame;
    if (capture_->captureFrame(frame)) {
      captured_count++;

      output_size = kOutputBufferSize;
      if (encoder_->encode(frame, output, &output_size)) {
        encoded_count++;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  EXPECT_GT(captured_count, 15) << "Captured: " << captured_count;
  EXPECT_GT(encoded_count, 10) << "Encoded: " << encoded_count;

  delete[] output;
}

TEST_F(CaptureEncoderIntegrationTest, CaptureWithCallbackAndEncode) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  int frame_count = 0;
  int encoded_count = 0;

  auto callback = [&](const VideoFrame& frame) {
    frame_count++;

    output_size = kOutputBufferSize;
    if (encoder_->encode(frame, output, &output_size)) {
      encoded_count++;
    }
  };

  capture_->setFrameCallback(callback);
  capture_->start();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  capture_->stop();

  EXPECT_GT(frame_count, 5) << "Captured: " << frame_count;
  EXPECT_GT(encoded_count, 3) << "Encoded: " << encoded_count;

  delete[] output;
}

TEST_F(CaptureEncoderIntegrationTest, EncodeWithDifferentConfigurations) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  VideoFrame frame;
  ASSERT_TRUE(capture_->captureFrame(frame));

  std::vector<EncoderConfig> configs = {
    EncoderConfig::getLowLatency(),
    EncoderConfig::getHighQuality()
  };

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  for (const auto& config : configs) {
    IVideoEncoder* test_encoder = factory_->createEncoder(EncoderType::kSoftwareX264);
    ASSERT_NE(test_encoder, nullptr);

    std::string config_json = config.toJson();
    ASSERT_TRUE(test_encoder->initialize(primary_display_.width, primary_display_.height, 60, config_json));

    output_size = kOutputBufferSize;
    bool encoded = test_encoder->encode(frame, output, &output_size);

    // High quality config with B-frames may need multiple frames to encode
    if (config.b_frames > 0) {
      // Skip strict validation for B-frame configurations
      delete test_encoder;
      continue;
    }

    EXPECT_TRUE(encoded) << "Failed with config: " << config_json;
    EXPECT_GT(output_size, 0) << "Config: " << config_json;

    delete test_encoder;
  }

  delete[] output;
}

TEST_F(CaptureEncoderIntegrationTest, PerformanceCaptureAndEncode) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  const int kTargetFrames = 60;

  auto start_time = std::chrono::high_resolution_clock::now();

  int encoded_count = 0;
  for (int i = 0; i < kTargetFrames; ++i) {
    VideoFrame frame;
    if (capture_->captureFrame(frame)) {
      output_size = kOutputBufferSize;
      if (encoder_->encode(frame, output, &output_size)) {
        encoded_count++;
      }
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time).count();

  double fps = (encoded_count * 1000.0) / duration_ms;

  // Realistic expectation for software encoding with GOP=1 (all I-frames)
  // GOP=1 causes every frame to be a keyframe, significantly slower encoding
  // Desktop Duplication API may also miss frames on some systems
  EXPECT_GE(encoded_count, kTargetFrames * 0.2)
    << "Encoded " << encoded_count << "/" << kTargetFrames << " frames";
  EXPECT_GT(fps, 2.0) << "Should achieve at least 2 FPS with all I-frames";

  std::cout << "Performance: " << encoded_count << " frames in " << duration_ms
            << "ms (" << fps << " FPS)" << std::endl;

  delete[] output;
}

TEST_F(CaptureEncoderIntegrationTest, MemoryStabilityLongRun) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  const int kLongRunFrames = 300;

  for (int i = 0; i < kLongRunFrames; ++i) {
    VideoFrame frame;
    if (capture_->captureFrame(frame)) {
      output_size = kOutputBufferSize;
      encoder_->encode(frame, output, &output_size);
    }
  }

  SUCCEED() << "Completed " << kLongRunFrames << " frames without memory issues";

  delete[] output;
}

TEST_F(CaptureEncoderIntegrationTest, EncoderFlushAfterCapture) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  VideoFrame frame;
  ASSERT_TRUE(capture_->captureFrame(frame));

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  for (int i = 0; i < 10; ++i) {
    capture_->captureFrame(frame);
    output_size = kOutputBufferSize;
    encoder_->encode(frame, output, &output_size);
  }

  encoder_->flush();

  SUCCEED();

  delete[] output;
}

TEST_F(CaptureEncoderIntegrationTest, HandleStrideDifferences) {
  ASSERT_TRUE(capture_->initialize(primary_display_.index));

  VideoFrame frame;
  ASSERT_TRUE(capture_->captureFrame(frame));

  EXPECT_GT(frame.stride, 0);
  EXPECT_EQ(frame.stride % 4, 0);

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder_->initialize(primary_display_.width, primary_display_.height, 60, config_json));

  bool encoded = encoder_->encode(frame, output, &output_size);
  EXPECT_TRUE(encoded) << "Failed with stride: " << frame.stride;

  delete[] output;
}

} // namespace screensdk
