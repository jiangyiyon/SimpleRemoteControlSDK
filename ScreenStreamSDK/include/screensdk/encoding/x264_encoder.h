#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <mutex>

#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"
#include "screensdk/transport/video_source.h"

#include "../../../third_party/x264/include/x264.h"

namespace screensdk {
/**
 * @brief x264 software encoder implementation
 *
 * T038: Implement x264 software encoder
 * FR-003: Hardware encoding with software fallback
 *
 * Features:
 * - Supports BGRA format directly (no color conversion needed)
 * - Configurable B-frames (0-16)
 * - Low-latency mode with GOP=1
 * - Thread-safe encoding
 * - Memory management with RAII
 */
class X264EncoderImpl : public IVideoEncoder {
public:
  X264EncoderImpl();
  ~X264EncoderImpl() override;

  X264EncoderImpl(const X264EncoderImpl&) = delete;
  X264EncoderImpl& operator=(const X264EncoderImpl&) = delete;
  X264EncoderImpl(X264EncoderImpl&&) = delete;
  X264EncoderImpl& operator=(X264EncoderImpl&&) = delete;

  bool initialize(int width, int height, int fps,
                 const std::string& config) override;

  bool encode(const VideoFrameForTrans& frame,
              uint8_t* output, size_t* output_size) override;

  void flush() override;

  EncoderType getType() const override;

  bool isAvailable() const override;

private:
  x264_t* encoder_{nullptr};
  x264_param_t* param_{nullptr};
  x264_picture_t* pic_in_{nullptr};
  x264_picture_t* pic_out_{nullptr};

  int width_{0};
  int height_{0};
  int fps_{0};
  bool initialized_{false};
  mutable std::mutex mutex_;

  bool allocatePictures();
  void freePictures();
  bool setupParams();
  bool applyConfig(const std::string& config_json);
};

} // namespace screensdk
