#include <gtest/gtest.h>

#include "screensdk/encoding/x264_encoder.h"
#include "screensdk/encoding/encoder_config.h"

namespace screensdk {

class X264EncoderTest : public ::testing::Test {
protected:
  void SetUp() override {
    encoder = new X264EncoderImpl();
  }

  void TearDown() override {
    delete encoder;
    encoder = nullptr;
  }

  X264EncoderImpl* encoder{nullptr};
};

TEST_F(X264EncoderTest, IsAvailable) {
  EXPECT_TRUE(encoder->isAvailable());
}

TEST_F(X264EncoderTest, GetType) {
  EXPECT_EQ(encoder->getType(), EncoderType::kSoftwareX264);
}

TEST_F(X264EncoderTest, InitializeSuccess) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(1920, 1080, 60, config_json);
  EXPECT_TRUE(result);
}

TEST_F(X264EncoderTest, InitializeInvalidWidth) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(0, 1080, 60, config_json);
  EXPECT_FALSE(result);
}

TEST_F(X264EncoderTest, InitializeInvalidHeight) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(1920, 0, 60, config_json);
  EXPECT_FALSE(result);
}

TEST_F(X264EncoderTest, InitializeInvalidFps) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  bool result = encoder->initialize(1920, 1080, 0, config_json);
  EXPECT_FALSE(result);
}

TEST_F(X264EncoderTest, InitializeWithEmptyConfig) {
  bool result = encoder->initialize(1920, 1080, 60, "");
  EXPECT_TRUE(result);
}

TEST_F(X264EncoderTest, EncodeBeforeInitialize) {
  VideoFrameForTrans frame;
  frame.data = nullptr;
  frame.size = 0;
  frame.width = 1920;
  frame.height = 1080;

  uint8_t output[1024];
  size_t output_size = sizeof(output);

  bool result = encoder->encode(frame, output, &output_size);
  EXPECT_FALSE(result);
}

TEST_F(X264EncoderTest, EncodeNullFrame) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder->initialize(1920, 1080, 60, config_json));

  VideoFrameForTrans frame;
  frame.data = nullptr;
  frame.size = 0;
  frame.width = 1920;
  frame.height = 1080;

  uint8_t output[1024];
  size_t output_size = sizeof(output);

  bool result = encoder->encode(frame, output, &output_size);
  EXPECT_FALSE(result);
}

TEST_F(X264EncoderTest, EncodeNullOutput) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder->initialize(1920, 1080, 60, config_json));

  VideoFrameForTrans frame;
  const int kFrameSize = 1920 * 1080 * 4;
  frame.data = new uint8_t[kFrameSize];
  frame.size = kFrameSize;
  frame.width = 1920;
  frame.height = 1080;
  frame.stride = 1920 * 4;

  bool result = encoder->encode(frame, nullptr, nullptr);
  EXPECT_FALSE(result);

  delete[] frame.data;
}

TEST_F(X264EncoderTest, EncodeValidFrame) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder->initialize(1920, 1080, 60, config_json));

  const int kFrameSize = 1920 * 1080 * 4;
  uint8_t* frame_data = new uint8_t[kFrameSize];
  for (int i = 0; i < kFrameSize; i += 4) {
    frame_data[i] = 128;
    frame_data[i + 1] = 128;
    frame_data[i + 2] = 128;
    frame_data[i + 3] = 255;
  }

  VideoFrameForTrans frame;
  frame.data = frame_data;
  frame.size = kFrameSize;
  frame.width = 1920;
  frame.height = 1080;
  frame.stride = 1920 * 4;
  frame.timestamp_ms = 0;

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  bool result = encoder->encode(frame, output, &output_size);
  EXPECT_TRUE(result);
  EXPECT_GT(output_size, 0);
  EXPECT_LT(output_size, kOutputBufferSize);

  delete[] frame_data;
  delete[] output;
}

TEST_F(X264EncoderTest, EncodeMultipleFrames) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder->initialize(1920, 1080, 60, config_json));

  const int kFrameSize = 1920 * 1080 * 4;
  uint8_t* frame_data = new uint8_t[kFrameSize];

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  int success_count = 0;
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < kFrameSize; j += 4) {
      frame_data[j] = (i * 25) % 256;
      frame_data[j + 1] = (i * 25) % 256;
      frame_data[j + 2] = (i * 25) % 256;
      frame_data[j + 3] = 255;
    }

    VideoFrameForTrans frame;
    frame.data = frame_data;
    frame.size = kFrameSize;
    frame.width = 1920;
    frame.height = 1080;
    frame.stride = 1920 * 4;
    frame.timestamp_ms = i * 16;

    output_size = kOutputBufferSize;
    if (encoder->encode(frame, output, &output_size)) {
      success_count++;
    }
  }

  EXPECT_GE(success_count, 5);

  delete[] frame_data;
  delete[] output;
}

TEST_F(X264EncoderTest, EncodeWithCustomConfig) {
  EncoderConfig config;
  config.width = 1280;
  config.height = 720;
  config.fps = 30;
  config.gop_size = 30;
  config.b_frames = 2;
  config.bitrate = 3000000;
  config.quality = 25;
  config.preset = "fast";
  config.tune = "zerolatency";

  std::string config_json = config.toJson();

  bool result = encoder->initialize(1280, 720, 30, config_json);
  EXPECT_TRUE(result);
}

TEST_F(X264EncoderTest, FlushEncoder) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder->initialize(1920, 1080, 60, config_json));

  encoder->flush();

  SUCCEED();
}

TEST_F(X264EncoderTest, FlushBeforeInitialize) {
  encoder->flush();

  SUCCEED();
}

TEST_F(X264EncoderTest, InitializeTwice) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder->initialize(1920, 1080, 60, config_json));

  bool result = encoder->initialize(1920, 1080, 60, config_json);
  EXPECT_FALSE(result);
}

TEST_F(X264EncoderTest, EncodeWithBFrameConfig) {
  EncoderConfig config = EncoderConfig::getHighQuality();
  std::string config_json = config.toJson();

  ASSERT_TRUE(encoder->initialize(1920, 1080, 60, config_json));

  const int kFrameSize = 1920 * 1080 * 4;
  uint8_t* frame_data = new uint8_t[kFrameSize];
  std::memset(frame_data, 128, kFrameSize);

  const size_t kOutputBufferSize = 10 * 1024 * 1024;
  uint8_t* output = new uint8_t[kOutputBufferSize];
  size_t output_size = kOutputBufferSize;

  int success_count = 0;
  for (int i = 0; i < 20; ++i) {
    VideoFrameForTrans frame;
    frame.data = frame_data;
    frame.size = kFrameSize;
    frame.width = 1920;
    frame.height = 1080;
    frame.stride = 1920 * 4;
    frame.timestamp_ms = i * 16;

    output_size = kOutputBufferSize;
    if (encoder->encode(frame, output, &output_size)) {
      success_count++;
    }
  }

  EXPECT_GT(success_count, 0);

  delete[] frame_data;
  delete[] output;
}

TEST_F(X264EncoderTest, EncodeWithDifferentResolutions) {
  EncoderConfig config = EncoderConfig::getLowLatency();
  std::string config_json = config.toJson();

  std::vector<std::pair<int, int>> resolutions = {
    {640, 480}, {1280, 720}, {1920, 1080}
  };

  for (auto [width, height] : resolutions) {
    auto* test_encoder = new X264EncoderImpl();
    ASSERT_TRUE(test_encoder->initialize(width, height, 60, config_json));

    const int kFrameSize = width * height * 4;
    uint8_t* frame_data = new uint8_t[kFrameSize];
    std::memset(frame_data, 128, kFrameSize);

    const size_t kOutputBufferSize = 10 * 1024 * 1024;
    uint8_t* output = new uint8_t[kOutputBufferSize];
    size_t output_size = kOutputBufferSize;

    VideoFrameForTrans frame;
    frame.data = frame_data;
    frame.size = kFrameSize;
    frame.width = width;
    frame.height = height;
    frame.stride = width * 4;
    frame.timestamp_ms = 0;

    bool result = test_encoder->encode(frame, output, &output_size);
    EXPECT_TRUE(result) << "Failed for resolution: " << width << "x" << height;

    delete[] frame_data;
    delete[] output;
    delete test_encoder;
  }
}

} // namespace screensdk

