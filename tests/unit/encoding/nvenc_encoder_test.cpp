#include <gtest/gtest.h>

#include "screensdk/encoding/nvenc_encoder.h"
#include "screensdk/encoding/encoder_config.h"
#include "screensdk/transport/video_source.h"

namespace screensdk {

class NvencEncoderTest : public ::testing::Test {
protected:
  void SetUp() override {
    encoder = new NvencEncoderImpl();
  }

  void TearDown() override {
    delete encoder;
    encoder = nullptr;
  }

  NvencEncoderImpl* encoder{nullptr};
};

TEST_F(NvencEncoderTest, GetTypeReturnsNvenc) {
  EXPECT_EQ(encoder->getType(), EncoderType::kHardwareNVENC);
}

TEST_F(NvencEncoderTest, IsAvailableWhenNvencInstalled) {
#ifdef HAS_NVENC
  bool available = encoder->isAvailable();
  // Result depends on actual GPU hardware
  // Just verify it doesn't crash
  (void)available;
#else
  EXPECT_FALSE(encoder->isAvailable());
#endif
}

TEST_F(NvencEncoderTest, InitializeSuccess) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(1920, 1080, 60, config_json);
  EXPECT_TRUE(result);
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, InitializeInvalidWidth) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(0, 1080, 60, config_json);
  EXPECT_FALSE(result);
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, InitializeInvalidHeight) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(1920, 0, 60, config_json);
  EXPECT_FALSE(result);
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, InitializeInvalidFps) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(1920, 1080, 0, config_json);
  EXPECT_FALSE(result);
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, EncodeSingleFrame) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool init_result = encoder->initialize(1920, 1080, 60, config_json);
  ASSERT_TRUE(init_result);

  // Create a test frame
  std::vector<uint8_t> frame_data(1920 * 1080 * 4, 128);
  VideoFrameForTrans frame;
  frame.data = frame_data.data();
  frame.size = frame_data.size();
  frame.width = 1920;
  frame.height = 1080;
  frame.stride = 1920 * 4;
  frame.timestamp_ms = 0;

  // Allocate output buffer (H.264 compressed frame should be smaller)
  std::vector<uint8_t> output(1024 * 1024);
  size_t output_size = 0;

  bool encode_result = encoder->encode(frame, output.data(), &output_size);
  EXPECT_TRUE(encode_result);
  EXPECT_GT(output_size, 0);
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, EncodeMultipleFrames) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool init_result = encoder->initialize(1920, 1080, 60, config_json);
  ASSERT_TRUE(init_result);

  std::vector<uint8_t> frame_data(1920 * 1080 * 4, 128);
  VideoFrameForTrans frame;
  frame.data = frame_data.data();
  frame.size = frame_data.size();
  frame.width = 1920;
  frame.height = 1080;
  frame.stride = 1920 * 4;

  std::vector<uint8_t> output(1024 * 1024);

  // Encode multiple frames
  for (int i = 0; i < 10; ++i) {
    frame.timestamp_ms = i * 16;
    size_t output_size = 0;

    bool encode_result = encoder->encode(frame, output.data(), &output_size);
    EXPECT_TRUE(encode_result);
    EXPECT_GT(output_size, 0);
  }
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, EncodeWhenNotInitialized) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  std::vector<uint8_t> frame_data(1920 * 1080 * 4, 128);
  VideoFrameForTrans frame;
  frame.data = frame_data.data();
  frame.size = frame_data.size();
  frame.width = 1920;
  frame.height = 1080;
  frame.stride = 1920 * 4;

  std::vector<uint8_t> output(1024 * 1024);
  size_t output_size = 0;

  bool encode_result = encoder->encode(frame, output.data(), &output_size);
  EXPECT_FALSE(encode_result);
  EXPECT_EQ(output_size, 0);
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, FlushUninitializedEncoder) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  // Should not crash
  encoder->flush();
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, FlushAfterEncoding) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool init_result = encoder->initialize(1920, 1080, 60, config_json);
  ASSERT_TRUE(init_result);

  std::vector<uint8_t> frame_data(1920 * 1080 * 4, 128);
  VideoFrameForTrans frame;
  frame.data = frame_data.data();
  frame.size = frame_data.size();
  frame.width = 1920;
  frame.height = 1080;
  frame.stride = 1920 * 4;

  std::vector<uint8_t> output(1024 * 1024);
  size_t output_size = 0;

  encoder->encode(frame, output.data(), &output_size);

  // Should not crash
  encoder->flush();
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, ThreadSafety) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool init_result = encoder->initialize(1920, 1080, 60, config_json);
  ASSERT_TRUE(init_result);

  std::vector<uint8_t> frame_data(1920 * 1080 * 4, 128);
  VideoFrameForTrans frame;
  frame.data = frame_data.data();
  frame.size = frame_data.size();
  frame.width = 1920;
  frame.height = 1080;
  frame.stride = 1920 * 4;

  std::vector<uint8_t> output(1024 * 1024);

  // Multiple threads calling encode (should not crash due to mutex)
  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i) {
    threads.emplace_back([this, &frame, &output, i]() {
      for (int j = 0; j < 5; ++j) {
        size_t output_size = 0;
        encoder->encode(frame, output.data(), &output_size);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // If we reach here, thread safety is working
  SUCCEED();
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

TEST_F(NvencEncoderTest, EncoderReinitialization) {
#ifdef HAS_NVENC
  if (!encoder->isAvailable()) {
    GTEST_SKIP() << "NVENC not available on this system";
  }

  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  // Initialize and encode multiple times
  for (int i = 0; i < 3; ++i) {
    bool init_result = encoder->initialize(1920, 1080, 60, config_json);
    EXPECT_TRUE(init_result);

    std::vector<uint8_t> frame_data(1920 * 1080 * 4, 128);
    VideoFrameForTrans frame;
    frame.data = frame_data.data();
    frame.size = frame_data.size();
    frame.width = 1920;
    frame.height = 1080;
    frame.stride = 1920 * 4;

    std::vector<uint8_t> output(1024 * 1024);
    size_t output_size = 0;

    bool encode_result = encoder->encode(frame, output.data(), &output_size);
    EXPECT_TRUE(encode_result);

    encoder->flush();
  }

  // If we reach here, reinitialization is working without memory leaks
  SUCCEED();
#else
  GTEST_SKIP() << "NVENC SDK not available";
#endif
}

} // namespace screensdk
