#define SCREEN_STREAM_SDK_EXPORTS
#include "screensdk/encoding/encoder_factory.h"

#include "screensdk/encoding/x264_encoder.h"
#include "screensdk/platform/gpu_detector.h"

namespace screensdk {

class EncoderFactoryImpl : public IEncoderFactory {
public:
  EncoderFactoryImpl();
  ~EncoderFactoryImpl() override = default;

  EncoderFactoryImpl(const EncoderFactoryImpl&) = delete;
  EncoderFactoryImpl& operator=(const EncoderFactoryImpl&) = delete;
  EncoderFactoryImpl(EncoderFactoryImpl&&) = delete;
  EncoderFactoryImpl& operator=(EncoderFactoryImpl&&) = delete;

  IVideoEncoder* createEncoder() override;
  IVideoEncoder* createEncoder(EncoderType type) override;
  bool hasHardwareEncoder() const override;
  EncoderType getBestEncoderType() const override;
  std::string getEncoderTypeName(EncoderType type) const override;

private:
  mutable bool gpu_detector_initialized_{false};
  mutable bool has_nvenc_{false};
  mutable bool has_quicksync_{false};

  void detectGpuCapabilities() const;
};

EncoderFactoryImpl::EncoderFactoryImpl() {
  detectGpuCapabilities();
}

void EncoderFactoryImpl::detectGpuCapabilities() const {
  if (gpu_detector_initialized_) {
    return;
  }

  auto* gpu_detector = CreateGpuDetector();
  if (gpu_detector != nullptr) {
    has_nvenc_ = gpu_detector->isNvenconline();
    has_quicksync_ = gpu_detector->isQuickSyncAvailable();
    DestroyGpuDetector(gpu_detector);
  }

  gpu_detector_initialized_ = true;
}

IVideoEncoder* EncoderFactoryImpl::createEncoder() {
  EncoderType type = getBestEncoderType();
  return createEncoder(type);
}

IVideoEncoder* EncoderFactoryImpl::createEncoder(EncoderType type) {
  switch (type) {
    case EncoderType::kSoftwareX264:
      return new X264EncoderImpl();

    case EncoderType::kHardwareNVENC:
      // NVENC encoder not implemented yet, fallback to software
      if (!has_nvenc_) {
        return new X264EncoderImpl();
      }
      // TODO: Implement NVENC encoder. For now, fallback to software
      return new X264EncoderImpl();

    case EncoderType::kHardwareQuickSync:
      // QuickSync encoder not implemented yet, fallback to software
      if (!has_quicksync_) {
        return new X264EncoderImpl();
      }
      // TODO: Implement QuickSync encoder. For now, fallback to software
      return new X264EncoderImpl();

    default:
      return nullptr;
  }
}

bool EncoderFactoryImpl::hasHardwareEncoder() const {
  return has_nvenc_ || has_quicksync_;
}

EncoderType EncoderFactoryImpl::getBestEncoderType() const {
  detectGpuCapabilities();

  if (has_nvenc_) {
    return EncoderType::kHardwareNVENC;
  }
  if (has_quicksync_) {
    return EncoderType::kHardwareQuickSync;
  }
  return EncoderType::kSoftwareX264;
}

std::string EncoderFactoryImpl::getEncoderTypeName(EncoderType type) const {
  switch (type) {
    case EncoderType::kHardwareNVENC:
      return "NVENC (NVIDIA)";
    case EncoderType::kHardwareQuickSync:
      return "QuickSync (Intel)";
    case EncoderType::kSoftwareX264:
      return "x264 Software";
    default:
      return "Unknown";
  }
}

extern "C" SCREEN_STREAM_SDK_EXPORT IEncoderFactory* CreateEncoderFactory() {
  return new EncoderFactoryImpl();
}

extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyEncoderFactory(
    IEncoderFactory* factory) {
  if (factory != nullptr) {
    delete factory;
  }
}

} // namespace screensdk
