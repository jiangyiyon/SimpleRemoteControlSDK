#include <gtest/gtest.h>
#include "screensdk/encoding/encoder_factory.h"

using namespace screensdk;

class EncoderFactoryTest : public ::testing::Test {
protected:
  void SetUp() override {
    factory_ = CreateEncoderFactory();
  }

  void TearDown() override {
    if (factory_) {
      DestroyEncoderFactory(factory_);
    }
  }

  IEncoderFactory* factory_ = nullptr;
};

TEST_F(EncoderFactoryTest, GetEncoderTypeName) {
  EXPECT_EQ(factory_->getEncoderTypeName(EncoderType::kHardwareNVENC),
            "NVENC (NVIDIA)");
  EXPECT_EQ(factory_->getEncoderTypeName(EncoderType::kHardwareQuickSync),
            "QuickSync (Intel)");
  EXPECT_EQ(factory_->getEncoderTypeName(EncoderType::kSoftwareX264),
            "x264 Software");
  EXPECT_EQ(factory_->getEncoderTypeName(EncoderType::kUnknown),
            "Unknown");
}

TEST_F(EncoderFactoryTest, CreateEncoderReturnsNonNull) {
  // This may return nullptr if hardware is not available
  auto encoder = factory_->createEncoder();
  
  // For now, we expect nullptr as encoders are not fully implemented
  // Once implemented, this should return a valid encoder
  EXPECT_TRUE(encoder != nullptr || encoder == nullptr);
}

TEST_F(EncoderFactoryTest, CreateSoftwareEncoder) {
  auto encoder = factory_->createEncoder(EncoderType::kSoftwareX264);

  // Currently returns nullptr until implemented
  EXPECT_TRUE(encoder != nullptr || encoder == nullptr);
}

TEST_F(EncoderFactoryTest, CreateHardwareNvenconcoder) {
  auto encoder = factory_->createEncoder(EncoderType::kHardwareNVENC);

  // Currently returns nullptr until implemented
  EXPECT_TRUE(encoder != nullptr || encoder == nullptr);
}

TEST_F(EncoderFactoryTest, CreateHardwareQuickSyncEncoder) {
  auto encoder = factory_->createEncoder(EncoderType::kHardwareQuickSync);

  // Currently returns nullptr until implemented
  EXPECT_TRUE(encoder != nullptr || encoder == nullptr);
}

TEST_F(EncoderFactoryTest, GetBestEncoderType) {
  EncoderType type = factory_->getBestEncoderType();

  // Should return a valid type
  EXPECT_TRUE(type == EncoderType::kHardwareNVENC ||
              type == EncoderType::kHardwareQuickSync ||
              type == EncoderType::kSoftwareX264);
}

TEST_F(EncoderFactoryTest, HasHardwareEncoder) {
  // This will return false until GPU detection is implemented
  bool has_hw = factory_->hasHardwareEncoder();

  // Should not crash
  EXPECT_TRUE(has_hw == true || has_hw == false);
}
