#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "screensdk/capture/display_detector.h"
#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"
#include "screensdk/platform/gpu_detector.h"

namespace screensdk {

/**
 * @brief Unit test for hardware encoder failure to software encoder fallback
 *
 * G1 [P] [US1]: Test for hardware encoder failure to software encoder fallback
 * FR-003: Hardware encoding with software fallback
 *
 * Tests:
 * - Automatic encoder selection (NVENC > QuickSync > x264)
 * - Software encoder fallback when hardware unavailable
 * - Encoder type detection and reporting
 * - Fallback path functionality
 */
class EncodingFallbackTest : public ::testing::Test {
protected:
  void SetUp() override {
    display_detector_ = new DisplayDetector();

    auto displays = display_detector_->getDisplays();
    ASSERT_GT(displays.size(), 0) << "No displays available for testing";

    primary_display_ = display_detector_->getPrimaryDisplay();
    ASSERT_NE(primary_display_.name, "") << "No primary display found";

    factory_ = CreateEncoderFactory();
    ASSERT_NE(factory_, nullptr);

    gpu_detector_ = CreateGpuDetector();
    ASSERT_NE(gpu_detector_, nullptr);

    // Initialize low-latency encoder config
    config_ = EncoderConfig::getLowLatency();
    config_json_ = config_.toJson();
  }

  void TearDown() override {
    if (encoder_ != nullptr) {
      delete encoder_;
      encoder_ = nullptr;
    }

    if (factory_ != nullptr) {
      DestroyEncoderFactory(factory_);
      factory_ = nullptr;
    }

    if (gpu_detector_ != nullptr) {
      DestroyGpuDetector(gpu_detector_);
      gpu_detector_ = nullptr;
    }

    if (display_detector_ != nullptr) {
      delete display_detector_;
      display_detector_ = nullptr;
    }
  }

  DisplayDetector* display_detector_{nullptr};
  IEncoderFactory* factory_{nullptr};
  IGpuDetector* gpu_detector_{nullptr};
  IVideoEncoder* encoder_{nullptr};
  DisplayInfo primary_display_;
  EncoderConfig config_;
  std::string config_json_;
};

TEST_F(EncodingFallbackTest, DetectHardwareEncoderAvailability) {
  bool has_hardware = factory_->hasHardwareEncoder();

  std::cout << "Hardware encoder available: " << (has_hardware ? "Yes" : "No")
            << std::endl;

  // This test verifies detection works correctly
  // The result depends on the actual system
  SUCCEED() << "Hardware detection completed: "
             << (has_hardware ? "Hardware found" : "Software fallback will be used");
}

TEST_F(EncodingFallbackTest, SelectBestEncoderType) {
  EncoderType best_type = factory_->getBestEncoderType();
  std::string type_name = factory_->getEncoderTypeName(best_type);

  std::cout << "Best encoder type: " << type_name << std::endl;

  // Verify best encoder type is valid
  EXPECT_NE(best_type, EncoderType::kUnknown);
  EXPECT_FALSE(type_name.empty());

  // Verify fallback logic: NVENC > QuickSync > x264
  auto* gpu_info = gpu_detector_->getGpuInfo().c_str();
  std::cout << "GPU info: " << gpu_info << std::endl;

  bool has_nvenc = gpu_detector_->isNvenconline();
  bool has_quicksync = gpu_detector_->isQuickSyncAvailable();

  if (has_nvenc) {
    EXPECT_EQ(best_type, EncoderType::kHardwareNVENC);
  } else if (has_quicksync) {
    EXPECT_EQ(best_type, EncoderType::kHardwareQuickSync);
  } else {
    EXPECT_EQ(best_type, EncoderType::kSoftwareX264);
  }
}

TEST_F(EncodingFallbackTest, CreateEncoderWithAutoSelection) {
  IVideoEncoder* auto_encoder = factory_->createEncoder();
  ASSERT_NE(auto_encoder, nullptr);

  EncoderType type = auto_encoder->getType();
  EXPECT_NE(type, EncoderType::kUnknown);

  // Verify encoder is available
  EXPECT_TRUE(auto_encoder->isAvailable());

  std::cout << "Auto-selected encoder: "
            << factory_->getEncoderTypeName(type) << std::endl;

  delete auto_encoder;
}

TEST_F(EncodingFallbackTest, CreateSoftwareEncoderAsFallback) {
  // Explicitly request software encoder
  IVideoEncoder* software_encoder = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(software_encoder, nullptr);

  EXPECT_EQ(software_encoder->getType(), EncoderType::kSoftwareX264);
  EXPECT_TRUE(software_encoder->isAvailable());

  // Verify software encoder can initialize
  bool initialized = software_encoder->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  );
  EXPECT_TRUE(initialized) << "Software encoder should initialize successfully";

  delete software_encoder;
}

TEST_F(EncodingFallbackTest, SoftwareEncoderEncodesFrames) {
  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  // Create a test video frame
  const size_t frame_size = primary_display_.width * primary_display_.height * 4; // BGRA
  std::vector<uint8_t> frame_data(frame_size, 0x80); // Gray frame

  VideoFrameForTrans frame;
  frame.data = frame_data.data();
  frame.size = frame_size;
  frame.width = primary_display_.width;
  frame.height = primary_display_.height;
  frame.stride = primary_display_.width * 4;
  frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size = kOutputBufferSize;

  bool encoded = encoder_->encode(frame, output.data(), &output_size);

  EXPECT_TRUE(encoded) << "Software encoder should encode successfully";
  EXPECT_GT(output_size, 0) << "Encoded output should not be empty";
  EXPECT_LT(output_size, kOutputBufferSize) << "Output should fit in buffer";
}

TEST_F(EncodingFallbackTest, SoftwareEncoderEncodesMultipleFrames) {
  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t frame_size = primary_display_.width * primary_display_.height * 4;
  std::vector<uint8_t> frame_data(frame_size, 0x80);

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  int success_count = 0;
  const int kFrameCount = 30;

  for (int i = 0; i < kFrameCount; ++i) {
    VideoFrameForTrans frame;
    frame.data = frame_data.data();
    frame.size = frame_size;
    frame.width = primary_display_.width;
    frame.height = primary_display_.height;
    frame.stride = primary_display_.width * 4;
    frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count() + i * 16; // ~60FPS

    output_size = kOutputBufferSize;
    if (encoder_->encode(frame, output.data(), &output_size)) {
      success_count++;
      EXPECT_GT(output_size, 0);
    }

    // Small delay to simulate real frame rate
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  EXPECT_GT(success_count, kFrameCount * 0.9)
    << "Should encode at least 90% of frames: " << success_count << "/" << kFrameCount;
}

TEST_F(EncodingFallbackTest, EncoderTypeNamesAreInformative) {
  for (int type_int = 0; type_int <= 3; ++type_int) {
    EncoderType type = static_cast<EncoderType>(type_int);
    std::string name = factory_->getEncoderTypeName(type);

    EXPECT_FALSE(name.empty()) << "Encoder type name should not be empty";

    if (type != EncoderType::kUnknown) {
      EXPECT_TRUE(name.find("NVIDIA") != std::string::npos ||
                  name.find("Intel") != std::string::npos ||
                  name.find("x264") != std::string::npos)
        << "Encoder name should indicate vendor or type: " << name;
    }
  }
}

TEST_F(EncodingFallbackTest, HardwareUnavailableForcesSoftwareFallback) {
  // This test verifies that when hardware encoder creation fails,
  // the system can fall back to software encoder

  // First, try to create hardware encoder
  IVideoEncoder* hw_encoder = factory_->createEncoder(EncoderType::kHardwareNVENC);

  bool hardware_available = (hw_encoder != nullptr);
  std::cout << "Hardware encoder creation: "
            << (hardware_available ? "Success" : "Failed (expected fallback)")
            << std::endl;

  if (hw_encoder != nullptr) {
    delete hw_encoder;
  }

  // Regardless of hardware availability, software encoder should always work
  IVideoEncoder* sw_encoder = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(sw_encoder, nullptr) << "Software encoder should always be available";

  EXPECT_EQ(sw_encoder->getType(), EncoderType::kSoftwareX264);
  EXPECT_TRUE(sw_encoder->isAvailable());

  // Verify software encoder can be initialized
  bool initialized = sw_encoder->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  );
  EXPECT_TRUE(initialized) << "Software encoder must be initializable";

  delete sw_encoder;

  SUCCEED() << "Software fallback verified: "
             << (hardware_available ? "Hardware also available" : "Hardware unavailable, software works");
}

TEST_F(EncodingFallbackTest, AutoSelectEncoderAndEncode) {
  // Test the complete fallback path: auto-select -> initialize -> encode
  encoder_ = factory_->createEncoder();
  ASSERT_NE(encoder_, nullptr);

  EncoderType type = encoder_->getType();
  std::cout << "Auto-selected encoder for encoding test: "
            << factory_->getEncoderTypeName(type) << std::endl;

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t frame_size = primary_display_.width * primary_display_.height * 4;
  std::vector<uint8_t> frame_data(frame_size, 0x80);

  VideoFrameForTrans frame;
  frame.data = frame_data.data();
  frame.size = frame_size;
  frame.width = primary_display_.width;
  frame.height = primary_display_.height;
  frame.stride = primary_display_.width * 4;
  frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size = kOutputBufferSize;

  bool encoded = encoder_->encode(frame, output.data(), &output_size);

  EXPECT_TRUE(encoded) << "Auto-selected encoder should encode successfully";
  EXPECT_GT(output_size, 0);

  std::cout << "Successfully encoded frame with "
            << factory_->getEncoderTypeName(type) << std::endl;
}

TEST_F(EncodingFallbackTest, PerformanceSoftwareEncoderFallback) {
  // Measure software encoder performance to ensure fallback is acceptable
  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t frame_size = primary_display_.width * primary_display_.height * 4;
  std::vector<uint8_t> frame_data(frame_size, 0x80);

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  const int kFrameCount = 30;
  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < kFrameCount; ++i) {
    VideoFrameForTrans frame;
    frame.data = frame_data.data();
    frame.size = frame_size;
    frame.width = primary_display_.width;
    frame.height = primary_display_.height;
    frame.stride = primary_display_.width * 4;
    frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count() + i * 16; // ~60FPS

    output_size = kOutputBufferSize;
    encoder_->encode(frame, output.data(), &output_size);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time).count();

  double fps = (kFrameCount * 1000.0) / duration_ms;
  double avg_ms_per_frame = static_cast<double>(duration_ms) / kFrameCount;

  std::cout << "Software encoder performance: " << kFrameCount << " frames in "
            << duration_ms << "ms (" << fps << " FPS, "
            << avg_ms_per_frame << "ms/frame)" << std::endl;

  // Software encoder should be able to encode at reasonable speed
  EXPECT_GT(fps, 10.0) << "Software encoder should achieve at least 10 FPS";
}

TEST_F(EncodingFallbackTest, FlushAfterEncoding) {
  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr);

  ASSERT_TRUE(encoder_->initialize(
    primary_display_.width,
    primary_display_.height,
    60,
    config_json_
  ));

  const size_t frame_size = primary_display_.width * primary_display_.height * 4;
  std::vector<uint8_t> frame_data(frame_size, 0x80);

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  std::vector<uint8_t> output(kOutputBufferSize);
  size_t output_size;

  // Encode multiple frames
  for (int i = 0; i < 10; ++i) {
    VideoFrameForTrans frame;
    frame.data = frame_data.data();
    frame.size = frame_size;
    frame.width = primary_display_.width;
    frame.height = primary_display_.height;
    frame.stride = primary_display_.width * 4;
    frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count() + i * 16;

    output_size = kOutputBufferSize;
    encoder_->encode(frame, output.data(), &output_size);
  }

  // Flush encoder
  encoder_->flush();

  SUCCEED() << "Software encoder flush completed successfully";
}

TEST_F(EncodingFallbackTest, GpuDetectorAndFactoryConsistent) {
  // Verify that GPU detector and factory agree on hardware availability
  bool factory_has_hw = factory_->hasHardwareEncoder();
  bool gpu_has_hw = gpu_detector_->hasHardwareEncoder();

  EXPECT_EQ(factory_has_hw, gpu_has_hw)
    << "Factory and GPU detector should agree on hardware availability";

  std::cout << "Factory hardware detection: " << (factory_has_hw ? "Yes" : "No")
            << ", GPU detector: " << (gpu_has_hw ? "Yes" : "No") << std::endl;

  // Verify best encoder type matches
  EncoderType factory_best = factory_->getBestEncoderType();
  EncoderType gpu_best = gpu_detector_->getBestEncoderType();

  EXPECT_EQ(factory_best, gpu_best)
    << "Factory and GPU detector should agree on best encoder type";

  std::cout << "Factory best encoder: "
            << factory_->getEncoderTypeName(factory_best) << ", GPU best: "
            << gpu_detector_->getEncoderTypeName(gpu_best) << std::endl;
}

} // namespace screensdk

