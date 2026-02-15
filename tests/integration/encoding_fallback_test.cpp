#include <gtest/gtest.h>

#include <chrono>
#include <vector>
#include <thread>

#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"
#include "screensdk/platform/gpu_detector.h"
#include "screensdk/transport/video_source.h"

namespace screensdk {

/**
 * @brief Integration test for encoder fallback functionality
 *
 * Tests FR-003: Hardware encoding with software fallback
 *
 * Key Integration Points:
 * - IGpuDetector detects hardware encoder availability
 * - IEncoderFactory selects best available encoder
 * - Hardware encoder failure triggers software fallback
 * - Encoding pipeline continues after fallback
 *
 * Scenarios:
 * - GPU detection and best encoder selection
 * - Hardware encoder unavailable â†?software fallback
 * - Encoding success after fallback
 * - Performance comparison: hardware vs software
 * - Multiple encoder creation with fallback
 * - Encoder type verification
 */
class EncodingFallbackTest : public ::testing::Test {
protected:
  void SetUp() override {
    factory_ = CreateEncoderFactory();
    ASSERT_NE(factory_, nullptr) << "Encoder factory should be created";

    config_ = EncoderConfig::getLowLatency();
    config_json_ = config_.toJson();
  }

  void TearDown() override {
    if (encoder_ != nullptr) {
      encoder_->flush();
      delete encoder_;
      encoder_ = nullptr;
    }

    if (factory_ != nullptr) {
      DestroyEncoderFactory(factory_);
      factory_ = nullptr;
    }
  }

  /**
   * @brief Create a test video frame
   */
  VideoFrameForTrans createTestFrame(int width, int height) {
    VideoFrameForTrans frame;
    frame.width = width;
    frame.height = height;
    frame.stride = width * 4;  // BGRA = 4 bytes per pixel
    frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();

    // Allocate and fill with test data
    size_t data_size = frame.stride * height;
    frame.size = data_size;
    frame.data = new uint8_t[data_size];

    for (size_t i = 0; i < data_size; i += 4) {
      frame.data[i + 0] = 255;  // B
      frame.data[i + 1] = 0;    // G
      frame.data[i + 2] = 0;    // R
      frame.data[i + 3] = 255;  // A
    }

    return frame;
  }

  /**
   * @brief Free a test video frame
   */
  void freeTestFrame(VideoFrameForTrans& frame) {
    if (frame.data != nullptr) {
      delete[] frame.data;
      frame.data = nullptr;
      frame.size = 0;
    }
  }

  /**
   * @brief Encode a single frame
   */
  bool encodeFrame(IVideoEncoder* encoder, const VideoFrameForTrans& frame,
                   size_t* encoded_size = nullptr) {
    if (encoder == nullptr) {
      return false;
    }

    const size_t kOutputBufferSize = 10 * 1024 * 1024;
    std::vector<uint8_t> output(kOutputBufferSize);
    size_t output_size = kOutputBufferSize;

    bool success = encoder->encode(frame, output.data(), &output_size);

    if (encoded_size != nullptr && success) {
      *encoded_size = output_size;
    }

    return success;
  }

  /**
   * @brief Encode a single frame using member encoder_
   */
  bool encodeFrame(const VideoFrameForTrans& frame, size_t* encoded_size = nullptr) {
    return encodeFrame(encoder_, frame, encoded_size);
  }

  IEncoderFactory* factory_{nullptr};
  IVideoEncoder* encoder_{nullptr};
  EncoderConfig config_;
  std::string config_json_;
};

TEST_F(EncodingFallbackTest, GpuDetectionAndBestEncoderSelection) {
  // Create GPU detector to check hardware availability
  auto* gpu_detector = CreateGpuDetector();
  ASSERT_NE(gpu_detector, nullptr) << "GPU detector should be created";

  bool has_nvenc = gpu_detector->isNvenconline();
  bool has_quicksync = gpu_detector->isQuickSyncAvailable();
  bool has_hardware = gpu_detector->hasHardwareEncoder();

  std::cout << "GPU Detection:" << std::endl;
  std::cout << "  NVENC: " << (has_nvenc ? "Available" : "Not available") << std::endl;
  std::cout << "  QuickSync: " << (has_quicksync ? "Available" : "Not available") << std::endl;
  std::cout << "  Hardware encoder: " << (has_hardware ? "Available" : "Not available") << std::endl;

  // Get best encoder type from factory
  EncoderType best_type = factory_->getBestEncoderType();
  std::string type_name = factory_->getEncoderTypeName(best_type);

  std::cout << "  Best encoder type: " << type_name << std::endl;

  // Verify factory has hardware encoder if GPU detector says so
  bool factory_has_hw = factory_->hasHardwareEncoder();
  EXPECT_EQ(has_hardware, factory_has_hw)
    << "Factory and GPU detector should agree on hardware availability";

  DestroyGpuDetector(gpu_detector);

  // If hardware is available, best type should be hardware
  if (has_hardware) {
    EXPECT_TRUE(best_type == EncoderType::kHardwareNVENC ||
                best_type == EncoderType::kHardwareQuickSync)
      << "Best encoder should be hardware when available";
  } else {
    EXPECT_EQ(best_type, EncoderType::kSoftwareX264)
      << "Best encoder should be software when hardware unavailable";
  }
}

TEST_F(EncodingFallbackTest, AutomaticEncoderSelection) {
  // Create encoder with automatic selection
  encoder_ = factory_->createEncoder();
  ASSERT_NE(encoder_, nullptr) << "Encoder should be created";

  // Verify encoder type
  EncoderType type = encoder_->getType();
  std::string type_name = factory_->getEncoderTypeName(type);

  std::cout << "Created encoder type: " << type_name << std::endl;

  // Initialize encoder
  int width = 640;
  int height = 480;
  ASSERT_TRUE(encoder_->initialize(width, height, 60, config_json_))
    << "Encoder should initialize";

  // Verify encoder is available
  EXPECT_TRUE(encoder_->isAvailable()) << "Encoder should be available";

  // Encode a frame
  VideoFrameForTrans frame = createTestFrame(width, height);
  size_t encoded_size;
  ASSERT_TRUE(encodeFrame(frame, &encoded_size))
    << "Should encode frame successfully";

  EXPECT_GT(encoded_size, 0) << "Encoded size should be positive";

  std::cout << "Encoded frame size: " << encoded_size << " bytes" << std::endl;
}

TEST_F(EncodingFallbackTest, SoftwareEncoderAlwaysWorks) {
  // Create software encoder explicitly
  encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
  ASSERT_NE(encoder_, nullptr) << "Software encoder should be created";

  EXPECT_EQ(encoder_->getType(), EncoderType::kSoftwareX264)
    << "Should be software encoder";

  // Initialize
  int width = 640;
  int height = 480;
  ASSERT_TRUE(encoder_->initialize(width, height, 60, config_json_))
    << "Software encoder should initialize";

  // Encode multiple frames
  const int kFrameCount = 10;
  int success_count = 0;
  std::vector<size_t> encoded_sizes;

  for (int i = 0; i < kFrameCount; ++i) {
    VideoFrameForTrans frame = createTestFrame(width, height);
    size_t encoded_size;

    if (encodeFrame(frame, &encoded_size)) {
      success_count++;
      encoded_sizes.push_back(encoded_size);
    }

    freeTestFrame(frame);
  }

  EXPECT_EQ(success_count, kFrameCount) << "All frames should encode";

  // Verify all frames have reasonable sizes
  for (auto size : encoded_sizes) {
    EXPECT_GT(size, 100) << "Encoded size should be > 100 bytes";
    EXPECT_LT(size, 10 * 1024 * 1024) << "Encoded size should be < 10MB";
  }

  std::cout << "Successfully encoded " << success_count << "/" << kFrameCount << " frames"
            << std::endl;
}

TEST_F(EncodingFallbackTest, HardwareEncoderFallsBackToSoftware) {
  // Request hardware encoder
  auto* gpu_detector = CreateGpuDetector();
  bool has_nvenc = gpu_detector->isNvenconline();
  bool has_quicksync = gpu_detector->isQuickSyncAvailable();
  DestroyGpuDetector(gpu_detector);

  // Try NVENC if available
  if (has_nvenc) {
    encoder_ = factory_->createEncoder(EncoderType::kHardwareNVENC);
    ASSERT_NE(encoder_, nullptr) << "NVENC encoder request should return fallback";

    std::cout << "NVENC requested - returned: "
              << factory_->getEncoderTypeName(encoder_->getType()) << std::endl;

    // Should fallback to software (NVENC not implemented yet)
    // Note: When NVENC is implemented, this test may need adjustment
  }

  // Try QuickSync if available
  if (has_quicksync && encoder_ == nullptr) {
    encoder_ = factory_->createEncoder(EncoderType::kHardwareQuickSync);
    ASSERT_NE(encoder_, nullptr) << "QuickSync encoder request should return fallback";

    std::cout << "QuickSync requested - returned: "
              << factory_->getEncoderTypeName(encoder_->getType()) << std::endl;

    // Should fallback to software (QuickSync not implemented yet)
  }

  // If no hardware, just test software
  if (encoder_ == nullptr) {
    encoder_ = factory_->createEncoder(EncoderType::kSoftwareX264);
    ASSERT_NE(encoder_, nullptr) << "Software encoder should be created";
  }

  // Initialize and encode (should work regardless of fallback)
  int width = 640;
  int height = 480;
  ASSERT_TRUE(encoder_->initialize(width, height, 60, config_json_))
    << "Encoder should initialize after fallback";

  VideoFrameForTrans frame = createTestFrame(width, height);
  size_t encoded_size;
  ASSERT_TRUE(encodeFrame(frame, &encoded_size))
    << "Should encode after fallback";

  EXPECT_GT(encoded_size, 0) << "Encoding should work after fallback";

  freeTestFrame(frame);

  std::cout << "Encoding successful after fallback: " << encoded_size << " bytes"
            << std::endl;
}

TEST_F(EncodingFallbackTest, EncoderSelectionIsConsistent) {
  // Create multiple encoders, verify consistent selection
  const int kEncoderCount = 5;
  std::vector<EncoderType> types;

  for (int i = 0; i < kEncoderCount; ++i) {
    auto* enc = factory_->createEncoder();
    ASSERT_NE(enc, nullptr) << "Encoder " << i << " should be created";

    types.push_back(enc->getType());

    delete enc;
  }

  // All encoders should be the same type
  for (size_t i = 1; i < types.size(); ++i) {
    EXPECT_EQ(types[0], types[i])
      << "Encoder selection should be consistent";
  }

  std::cout << "All " << kEncoderCount << " encoders selected same type: "
            << factory_->getEncoderTypeName(types[0]) << std::endl;
}

TEST_F(EncodingFallbackTest, EncodingPerformanceComparison) {
  // Compare encoding performance between selections
  const int kFrameCount = 30;
  const int kWidth = 1280;
  const int kHeight = 720;

  std::vector<std::pair<EncoderType, double>> encoding_times;

  // Test software encoder
  {
    auto* enc = factory_->createEncoder(EncoderType::kSoftwareX264);
    ASSERT_NE(enc, nullptr);
    ASSERT_TRUE(enc->initialize(kWidth, kHeight, 60, config_json_));

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < kFrameCount; ++i) {
      VideoFrameForTrans frame = createTestFrame(kWidth, kHeight);
      encodeFrame(enc, frame);
      freeTestFrame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
      end - start).count();

    double avg_ms = duration_us / 1000.0 / kFrameCount;
    encoding_times.push_back({EncoderType::kSoftwareX264, avg_ms});

    delete enc;

    std::cout << "Software encoder: " << avg_ms << "ms per frame ("
              << (1000.0 / avg_ms) << " FPS)" << std::endl;
  }

  // Test auto-selected encoder
  {
    auto* enc = factory_->createEncoder();
    ASSERT_NE(enc, nullptr);
    ASSERT_TRUE(enc->initialize(kWidth, kHeight, 60, config_json_));

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < kFrameCount; ++i) {
      VideoFrameForTrans frame = createTestFrame(kWidth, kHeight);
      encodeFrame(enc, frame);
      freeTestFrame(frame);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
      end - start).count();

    double avg_ms = duration_us / 1000.0 / kFrameCount;
    EncoderType type = enc->getType();
    encoding_times.push_back({type, avg_ms});

    delete enc;

    std::cout << "Auto-selected ("
              << factory_->getEncoderTypeName(type)
              << "): " << avg_ms << "ms per frame ("
              << (1000.0 / avg_ms) << " FPS)" << std::endl;
  }

  // Verify software encoder achieves reasonable performance
  double software_ms = encoding_times[0].second;
  EXPECT_LT(software_ms, 50.0) << "Software encoder should achieve > 20 FPS";
  EXPECT_GT(software_ms, 0.1) << "Software encoder should not be instant";
}

TEST_F(EncodingFallbackTest, MultipleEncoderTypesIndependent) {
  // Create encoders of different types, verify independence
  std::vector<IVideoEncoder*> encoders;

  // Create software encoder
  encoders.push_back(factory_->createEncoder(EncoderType::kSoftwareX264));
  encoders.push_back(factory_->createEncoder(EncoderType::kSoftwareX264));

  // Create auto-selected encoder
  encoders.push_back(factory_->createEncoder());

  // Initialize all
  int width = 640;
  int height = 480;

  for (auto* enc : encoders) {
    ASSERT_NE(enc, nullptr);
    ASSERT_TRUE(enc->initialize(width, height, 60, config_json_));
  }

  // Encode frames with each encoder
  std::vector<size_t> sizes;

  for (auto* enc : encoders) {
    VideoFrameForTrans frame = createTestFrame(width, height);
    size_t encoded_size;

    ASSERT_TRUE(encodeFrame(enc, frame, &encoded_size));
    sizes.push_back(encoded_size);

    freeTestFrame(frame);
  }

  // All should succeed
  EXPECT_EQ(sizes.size(), encoders.size()) << "All encoders should work";

  // All should have reasonable sizes
  for (auto size : sizes) {
    EXPECT_GT(size, 100);
    EXPECT_LT(size, 10 * 1024 * 1024);
  }

  // Clean up
  for (auto* enc : encoders) {
    delete enc;
  }

  std::cout << "Successfully encoded with " << encoders.size()
            << " independent encoders" << std::endl;
}

TEST_F(EncodingFallbackTest, EncoderHandlesDifferentResolutions) {
  // Test encoder handles resolution changes
  encoder_ = factory_->createEncoder();
  ASSERT_NE(encoder_, nullptr);

  std::vector<std::pair<int, int>> resolutions = {
    {640, 480},
    {1280, 720},
    {1920, 1080},
    {800, 600}
  };

  for (auto [w, h] : resolutions) {
    // Reinitialize for each resolution
    encoder_->flush();
    delete encoder_;

    encoder_ = factory_->createEncoder();
    ASSERT_TRUE(encoder_->initialize(w, h, 60, config_json_))
      << "Should initialize for " << w << "x" << h;

    // Encode frame
    VideoFrameForTrans frame = createTestFrame(w, h);
    size_t encoded_size;
    ASSERT_TRUE(encodeFrame(frame, &encoded_size))
      << "Should encode " << w << "x" << h;

    EXPECT_GT(encoded_size, 0) << w << "x" << h << " should produce output";

    freeTestFrame(frame);

    std::cout << "Encoded " << w << "x" << h << ": " << encoded_size << " bytes"
              << std::endl;
  }
}

TEST_F(EncodingFallbackTest, EncoderFailsGracefullyWithInvalidType) {
  // Request unknown encoder type
  encoder_ = factory_->createEncoder(EncoderType::kUnknown);

  // Should return nullptr
  EXPECT_EQ(encoder_, nullptr) << "Unknown encoder type should return nullptr";
}

TEST_F(EncodingFallbackTest, FactoryReportsHardwareAvailability) {
  bool has_hw = factory_->hasHardwareEncoder();

  // Should match GPU detector
  auto* gpu_detector = CreateGpuDetector();
  bool gpu_has_hw = gpu_detector->hasHardwareEncoder();

  EXPECT_EQ(has_hw, gpu_has_hw)
    << "Factory and GPU detector should agree on hardware availability";

  std::cout << "Hardware encoder available: " << (has_hw ? "Yes" : "No")
            << std::endl;

  DestroyGpuDetector(gpu_detector);

  // If hardware is available, verify best encoder is hardware
  if (has_hw) {
    EncoderType best = factory_->getBestEncoderType();
    EXPECT_TRUE(best == EncoderType::kHardwareNVENC ||
                best == EncoderType::kHardwareQuickSync)
      << "Best encoder should be hardware when available";
  }
}

} // namespace screensdk

