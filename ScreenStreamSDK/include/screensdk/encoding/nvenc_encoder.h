#pragma once

#include "screensdk/encoding/encoder_factory.h"

#include <mutex>

namespace screensdk {

/**
 * @brief NVIDIA NVENC hardware encoder implementation
 *
 * T038: Implement NVENC hardware encoder with low-latency settings
 * FR-003: Hardware encoding with software fallback
 *
 * Features:
 * - Low-latency encoding (GOP=1, B-frames=0)
 * - H.264 High Profile
 * - CQP rate control for constant quality
 * - D3D11 interop for zero-copy encoding
 * - Thread-safe operations
 * - Graceful fallback when NVENC unavailable
 */
class NvencEncoderImpl : public IVideoEncoder {
public:
  NvencEncoderImpl();
  ~NvencEncoderImpl() override;

  // Disable copy and move
  NvencEncoderImpl(const NvencEncoderImpl&) = delete;
  NvencEncoderImpl& operator=(const NvencEncoderImpl&) = delete;
  NvencEncoderImpl(NvencEncoderImpl&&) = delete;
  NvencEncoderImpl& operator=(NvencEncoderImpl&&) = delete;

  // IVideoEncoder interface implementation
  bool initialize(int width, int height, int fps,
                  const std::string& config) override;
  bool encode(const VideoFrameForTrans& frame,
              uint8_t* output, size_t* output_size) override;
  void flush() override;
  EncoderType getType() const override;
  bool isAvailable() const override;

private:
  // NVENC initialization and management
  bool initializeNvenc();
  bool createD3d11Device();
  bool loadNvencApi();
  void cleanup();

  // Configuration
  bool applyConfig(const std::string& config_json);

  // Member variables
  void* encoder_{nullptr};
  void* d3d_device_{nullptr};
  void* d3d_context_{nullptr};
  void* registered_ptr_{nullptr};
  void* input_buffer_{nullptr};
  void* output_buffer_{nullptr};

  int width_{0};
  int height_{0};
  int fps_{0};
  bool initialized_{false};
  bool nvenc_available_{false};

  mutable std::mutex mutex_;
};

} // namespace screensdk
