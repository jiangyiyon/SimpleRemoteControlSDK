#include <gtest/gtest.h>
#include "screensdk/encoding/encoder_config.h"

using namespace screensdk;

class EncoderConfigTest : public ::testing::Test {
};

TEST(EncoderConfigTest, GetDefaultConfigValid) {
  auto config = EncoderConfig::getDefault();

  EXPECT_GT(config.width, 0);
  EXPECT_GT(config.height, 0);
  EXPECT_GT(config.fps, 0);
  EXPECT_GT(config.bitrate, 0);
}

TEST(EncoderConfigTest, GetLowLatencyConfig) {
  auto config = EncoderConfig::getLowLatency();

  EXPECT_EQ(config.gop_size, 1);
  EXPECT_EQ(config.b_frames, 0);
  EXPECT_EQ(config.fps, 60);
  EXPECT_EQ(config.preset, "ultrafast");
  EXPECT_EQ(config.tune, "zerolatency");
}

TEST(EncoderConfigTest, GetHighQualityConfig) {
  auto config = EncoderConfig::getHighQuality();

  EXPECT_EQ(config.gop_size, 30);
  EXPECT_EQ(config.b_frames, 3);
  EXPECT_GT(config.bitrate, 5000000);
  EXPECT_EQ(config.preset, "fast");
  EXPECT_EQ(config.tune, "zerolatency");
}

TEST(EncoderConfigTest, ValidateDefaultConfig) {
  auto config = EncoderConfig::getDefault();

  EXPECT_TRUE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateValidConfig) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;
  config.gop_size = 1;
  config.b_frames = 0;
  config.bitrate = 5000000;

  EXPECT_TRUE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateInvalidWidth) {
  EncoderConfig config;
  config.width = -1;
  config.height = 1080;
  config.fps = 60;

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateTooLargeWidth) {
  EncoderConfig config;
  config.width = 10000;  // Max 4096
  config.height = 1080;
  config.fps = 60;

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateInvalidFps) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 0;

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateTooHighFps) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 200;  // Max 120

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateInvalidGopSize) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;
  config.gop_size = 0;

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateTooManyBFrames) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;
  config.b_frames = 20;  // Max 16

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateInvalidBitrate) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;
  config.bitrate = 100000;  // Min 1Mbps

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ValidateTooHighBitrate) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;
  config.bitrate = 30000000;  // Max 20Mbps

  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, ToJsonReturnsValidString) {
  auto config = EncoderConfig::getLowLatency();
  std::string json = config.toJson();

  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("width"), std::string::npos);
  EXPECT_NE(json.find("height"), std::string::npos);
  EXPECT_NE(json.find("fps"), std::string::npos);
}

TEST(EncoderConfigTest, FromJsonParsesCorrectly) {
  auto original = EncoderConfig::getLowLatency();
  std::string json = original.toJson();
  auto parsed = EncoderConfig::fromJson(json);

  EXPECT_EQ(parsed.width, original.width);
  EXPECT_EQ(parsed.height, original.height);
  EXPECT_EQ(parsed.fps, original.fps);
  EXPECT_EQ(parsed.gop_size, original.gop_size);
  EXPECT_EQ(parsed.b_frames, original.b_frames);
  EXPECT_EQ(parsed.bitrate, original.bitrate);
  EXPECT_EQ(parsed.quality, original.quality);
  EXPECT_EQ(parsed.preset, original.preset);
  EXPECT_EQ(parsed.tune, original.tune);
  EXPECT_EQ(parsed.use_hardware_encoder, original.use_hardware_encoder);
  EXPECT_EQ(parsed.allow_software_fallback, original.allow_software_fallback);
}

TEST(EncoderConfigTest, FromJsonWithInvalidInput) {
  auto config = EncoderConfig::fromJson("invalid json");

  EXPECT_NE(config.width, 0);
  EXPECT_NE(config.height, 0);
}

TEST(EncoderConfigTest, ToJsonFromJsonRoundtrip) {
  auto original = EncoderConfig::getHighQuality();
  std::string json = original.toJson();
  auto restored = EncoderConfig::fromJson(json);

  EXPECT_EQ(restored.width, original.width);
  EXPECT_EQ(restored.height, original.height);
  EXPECT_EQ(restored.fps, original.fps);
  EXPECT_EQ(restored.gop_size, original.gop_size);
  EXPECT_EQ(restored.b_frames, original.b_frames);
  EXPECT_EQ(restored.bitrate, original.bitrate);
  EXPECT_EQ(restored.quality, original.quality);
  EXPECT_EQ(restored.preset, original.preset);
  EXPECT_EQ(restored.tune, original.tune);
  EXPECT_EQ(restored.use_hardware_encoder, original.use_hardware_encoder);
  EXPECT_EQ(restored.allow_software_fallback, original.allow_software_fallback);
}

TEST(EncoderConfigTest, HardwareEncoderSettings) {
  auto config = EncoderConfig::getDefault();

  EXPECT_TRUE(config.use_hardware_encoder);
  EXPECT_TRUE(config.allow_software_fallback);
}

TEST(EncoderConfigTest, QualityRange) {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;

  // Test quality range (0-51)
  config.quality = -1;
  EXPECT_FALSE(validateEncoderConfig(config));

  config.quality = 0;
  EXPECT_TRUE(validateEncoderConfig(config));

  config.quality = 51;
  EXPECT_TRUE(validateEncoderConfig(config));

  config.quality = 52;
  EXPECT_FALSE(validateEncoderConfig(config));
}

TEST(EncoderConfigTest, CommonResolutions) {
  struct Resolution {
    int width;
    int height;
  };

  std::vector<Resolution> resolutions = {
    {640, 480},
    {1280, 720},
    {1920, 1080},
    {2560, 1440},
    {3840, 2160}
  };

  for (const auto& res : resolutions) {
    EncoderConfig config;
    config.width = res.width;
    config.height = res.height;
    config.fps = 60;

    EXPECT_TRUE(validateEncoderConfig(config))
        << "Resolution " << res.width << "x" << res.height << " should be valid";
  }
}
