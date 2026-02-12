#pragma once

#include <cstdint>
#include <string>

#include "screensdk/export.h"
#include "screensdk/encoding/encoder_factory.h"

namespace screensdk {

/**
 * @brief GPU capability detector interface
 *
 * T026: Implement GPU capability detection (NVENC/QuickSync)
 * FR-003: Hardware encoding with software fallback
 *
 * Detects available hardware encoders on the system:
 * - NVENC (NVIDIA GPU encoding)
 * - QuickSync (Intel GPU encoding)
 * Falls back to software encoding if none available
 */
struct IGpuDetector {
  virtual ~IGpuDetector() = default;

  /**
   * @brief Check if NVIDIA NVENC hardware encoder is available
   * @return true if NVENC is available
   */
  virtual bool isNvenconline() = 0;

  /**
   * @brief Check if Intel QuickSync hardware encoder is available
   * @return true if QuickSync is available
   */
  virtual bool isQuickSyncAvailable() = 0;

  /**
   * @brief Check if any hardware encoder is available
   * @return true if NVENC or QuickSync is available
   */
  virtual bool hasHardwareEncoder() = 0;

  /**
   * @brief Get GPU information string
   * @return Human-readable GPU information
   */
  virtual std::string getGpuInfo() = 0;

  /**
   * @brief Get the best available encoder type
   * @return EncoderType - prefers hardware over software
   */
  virtual EncoderType getBestEncoderType() = 0;

  /**
   * @brief Get encoder type name for display
   * @param type Encoder type
   * @return Human-readable encoder name
   */
  virtual std::string getEncoderTypeName(EncoderType type) = 0;
};

/**
 * @brief Factory function to create GPU detector
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IGpuDetector* CreateGpuDetector();

/**
 * @brief Factory function to destroy GPU detector
 */
extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyGpuDetector(IGpuDetector* detector);

} // namespace screensdk
