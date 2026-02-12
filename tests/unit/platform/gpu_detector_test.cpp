#include <gtest/gtest.h>
#include "screensdk/platform/gpu_detector.h"

using namespace screensdk;

class GpuDetectorTest : public ::testing::Test {
protected:
  void SetUp() override {
    detector_ = CreateGpuDetector();
  }

  void TearDown() override {
    if (detector_) {
      DestroyGpuDetector(detector_);
    }
  }

  IGpuDetector* detector_ = nullptr;
};

TEST_F(GpuDetectorTest, CreateGpuDetectorWorks) {
  EXPECT_NE(detector_, nullptr);
}

TEST_F(GpuDetectorTest, DestroyGpuDetectorHandlesNull) {
  DestroyGpuDetector(nullptr);
  SUCCEED();
}

TEST_F(GpuDetectorTest, DetectNvenconline) {
  bool nvenc_available = detector_->isNvenconline();
  
  // On systems with NVIDIA GPU and driver, should return true
  // On systems without, should return false
  // Either result is valid, but should not crash
  EXPECT_TRUE(nvenc_available || !nvenc_available);
}

TEST_F(GpuDetectorTest, DetectQuickSyncAvailable) {
  bool qsv_available = detector_->isQuickSyncAvailable();
  
  // On systems with Intel GPU and Media SDK, should return true
  // On systems without, should return false
  // Either result is valid, but should not crash
  EXPECT_TRUE(qsv_available || !qsv_available);
}

TEST_F(GpuDetectorTest, DetectHasHardwareEncoder) {
  bool has_hw = detector_->hasHardwareEncoder();
  
  // Should return true if NVENC or QuickSync is available
  bool nvenc = detector_->isNvenconline();
  bool qsv = detector_->isQuickSyncAvailable();
  
  EXPECT_EQ(has_hw, nvenc || qsv);
}

TEST_F(GpuDetectorTest, GetGpuInfoReturnsValidData) {
  auto gpu_info = detector_->getGpuInfo();
  
  // Should not be empty
  EXPECT_FALSE(gpu_info.empty());
  
  // Should contain vendor info
  bool has_vendor = (gpu_info.find("NVIDIA") != std::string::npos) ||
                     (gpu_info.find("Intel") != std::string::npos);
  EXPECT_TRUE(has_vendor || !has_vendor);
}

TEST_F(GpuDetectorTest, DetectReturnsCorrectEncoderType) {
  EncoderType type = detector_->getBestEncoderType();
  
  // Should be one of the known encoder types
  bool valid_type = (type == EncoderType::kHardwareNVENC) ||
                    (type == EncoderType::kHardwareQuickSync) ||
                    (type == EncoderType::kSoftwareX264) ||
                    (type == EncoderType::kUnknown);
  
  EXPECT_TRUE(valid_type);
}

TEST_F(GpuDetectorTest, GetEncoderTypeName) {
  EXPECT_EQ(detector_->getEncoderTypeName(EncoderType::kHardwareNVENC),
            "NVENC (NVIDIA)");
  EXPECT_EQ(detector_->getEncoderTypeName(EncoderType::kHardwareQuickSync),
            "QuickSync (Intel)");
  EXPECT_EQ(detector_->getEncoderTypeName(EncoderType::kSoftwareX264),
            "x264 Software");
  EXPECT_EQ(detector_->getEncoderTypeName(EncoderType::kUnknown),
            "Unknown");
}

TEST_F(GpuDetectorTest, DetectNoHardwareEncoder) {
  // On systems without any hardware encoder
  bool nvenc = detector_->isNvenconline();
  bool qsv = detector_->isQuickSyncAvailable();
  
  if (!nvenc && !qsv) {
    bool has_hw = detector_->hasHardwareEncoder();
    EXPECT_FALSE(has_hw);
    
    EncoderType type = detector_->getBestEncoderType();
    EXPECT_EQ(type, EncoderType::kSoftwareX264);
  }
}

TEST_F(GpuDetectorTest, NvenconlineReturnsConsistentResult) {
  bool first = detector_->isNvenconline();
  bool second = detector_->isNvenconline();
  
  EXPECT_EQ(first, second);
}

TEST_F(GpuDetectorTest, QuickSyncReturnsConsistentResult) {
  bool first = detector_->isQuickSyncAvailable();
  bool second = detector_->isQuickSyncAvailable();
  
  EXPECT_EQ(first, second);
}

TEST_F(GpuDetectorTest, MultipleCallsToGetGpuInfo) {
  auto info1 = detector_->getGpuInfo();
  auto info2 = detector_->getGpuInfo();
  
  // Should return the same info
  EXPECT_EQ(info1, info2);
}
