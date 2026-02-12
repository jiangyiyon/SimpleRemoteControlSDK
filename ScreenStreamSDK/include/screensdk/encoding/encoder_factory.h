#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "screensdk/export.h"
#include "screensdk/transport/video_source.h"

namespace screensdk {

/**
 * @brief Encoder type
 */
enum class EncoderType {
  kHardwareNVENC,
  kHardwareQuickSync,
  kSoftwareX264,
  kUnknown
};

/**
 * @brief Encoder interface
 */
struct IVideoEncoder {
  virtual ~IVideoEncoder() = default;

  /**
   * @brief Initialize encoder
   */
  virtual bool initialize(int width, int height, int fps,
                       const std::string& config) = 0;

  /**
   * @brief Encode a video frame
   * @param frame Input video frame
   * @param output Output buffer
   * @param output_size Output buffer size
   * @return true on success
   */
  virtual bool encode(const VideoFrame& frame,
                    uint8_t* output, size_t* output_size) = 0;

  /**
   * @brief Flush encoder
   */
  virtual void flush() = 0;

  /**
   * @brief Get encoder type
   */
  virtual EncoderType getType() const = 0;

  /**
   * @brief Check if encoder is available
   */
  virtual bool isAvailable() const = 0;
};

/**
 * @brief Encoder factory for hardware/software encoder selection
 *
 * T021: Implement encoder factory with hardware encoder/software encoder selection
 * FR-003: Hardware encoding with software fallback
 *
 * Automatically selects best available encoder:
 * 1. NVENC (NVIDIA GPU)
 * 2. QuickSync (Intel GPU)
 * 3. Software x264 (fallback)
 */
struct IEncoderFactory {
  virtual ~IEncoderFactory() = default;

  /**
   * @brief Create encoder instance with automatic selection
   */
  virtual IVideoEncoder* createEncoder() = 0;

  /**
   * @brief Create encoder of specific type
   */
  virtual IVideoEncoder* createEncoder(EncoderType type) = 0;

  /**
   * @brief Check if hardware encoder is available
   */
  virtual bool hasHardwareEncoder() const = 0;

  /**
   * @brief Get best available encoder type
   */
  virtual EncoderType getBestEncoderType() const = 0;

  /**
   * @brief Get encoder type name
   */
  virtual std::string getEncoderTypeName(EncoderType type) const = 0;
};

/**
 * @brief Factory function to create encoder factory
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IEncoderFactory* CreateEncoderFactory();
extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyEncoderFactory(IEncoderFactory* factory);

} // namespace screensdk
