#define SCREEN_STREAM_SDK_EXPORTS
#include "screensdk/encoding/encoder_factory.h"

namespace screensdk {

class EncoderFactoryImpl : public IEncoderFactory {
public:
  EncoderFactoryImpl() = default;
  ~EncoderFactoryImpl() override = default;

  EncoderFactoryImpl(const EncoderFactoryImpl&) = delete;
  EncoderFactoryImpl& operator=(const EncoderFactoryImpl&) = delete;
  EncoderFactoryImpl(EncoderFactoryImpl&&) = delete;
  EncoderFactoryImpl& operator=(EncoderFactoryImpl&&) = delete;

  IVideoEncoder* createEncoder() override {
    EncoderType type = getBestEncoderType();
    return createEncoder(type);
  }

  IVideoEncoder* createEncoder(EncoderType type) override {
    // TODO: Implement encoder instances
    // - T038: NVENC hardware encoder
    // - U1: Intel QuickSync encoder
    // - T039: Software x264 encoder
    (void)type;
    return nullptr;
  }

  bool hasHardwareEncoder() const override {
    // TODO: Check GPU availability
    // - T026: Implement GPU capability detection
    return false;
  }

  EncoderType getBestEncoderType() const override {
    // TODO: Implement GPU detection and encoder selection
    // Priority: NVENC > QuickSync > Software
    return EncoderType::kSoftwareX264;
  }

  std::string getEncoderTypeName(EncoderType type) const override {
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
};

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
