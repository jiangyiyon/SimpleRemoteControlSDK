#include "screensdk/encoding/nvenc_encoder.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#ifdef HAS_NVENC
#include "nvEncodeAPI.h"
#endif

// Windows min/max macro fix
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace screensdk {

NvencEncoderImpl::NvencEncoderImpl()
    : initialized_(false),
      nvenc_available_(false) {
#ifdef HAS_NVENC
  nvenc_available_ = initializeNvenc();
#endif
}

NvencEncoderImpl::~NvencEncoderImpl() {
  cleanup();
}

bool NvencEncoderImpl::initialize(int width, int height, int fps,
                                   const std::string& config) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!nvenc_available_) {
    return false;
  }

  if (width <= 0 || height <= 0 || fps <= 0) {
    return false;
  }

  width_ = width;
  height_ = height;
  fps_ = fps;

  // TODO: Parse config JSON and apply NVENC parameters
  // For now, use default low-latency settings

#ifdef HAS_NVENC
  // TODO: Reconfigure NVENC encoder with new parameters
  initialized_ = true;
  return true;
#else
  return false;
#endif
}

bool NvencEncoderImpl::encode(const VideoFrameForTrans& frame,
                                uint8_t* output, size_t* output_size) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_ || !nvenc_available_) {
    if (output_size) {
      *output_size = 0;
    }
    return false;
  }

  if (frame.data == nullptr || output == nullptr || output_size == nullptr) {
    return false;
  }

#ifdef HAS_NVENC
  // TODO: Implement NVENC encoding
  // 1. Map input buffer
  // 2. Copy frame data to NVENC input buffer
  // 3. Call NvEncEncodePicture
  // 4. Lock output buffer
  // 5. Copy output to output buffer
  // 6. Unlock output buffer
  // 7. Set output_size

  // Placeholder: copy frame data to output (for testing)
  size_t copy_size = std::min(frame.size, size_t(1024 * 1024));
  std::memcpy(output, frame.data, copy_size);
  *output_size = copy_size;

  return true;
#else
  return false;
#endif
}

void NvencEncoderImpl::flush() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_ || !nvenc_available_) {
    return;
  }

#ifdef HAS_NVENC
  // TODO: Call NvEncFlushEncoderBuffer
#endif
}

EncoderType NvencEncoderImpl::getType() const {
  return EncoderType::kHardwareNVENC;
}

bool NvencEncoderImpl::isAvailable() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return nvenc_available_;
}

bool NvencEncoderImpl::initializeNvenc() {
#ifdef HAS_NVENC
  if (!loadNvencApi()) {
    return false;
  }

  if (!createD3d11Device()) {
    return false;
  }

  // TODO: Initialize NVENC encoder
  // 1. Create NVENC instance
  // 2. Open encode session
  // 3. Allocate input and output buffers
  // 4. Configure encoder parameters

  return true;
#else
  return false;
#endif
}

bool NvencEncoderImpl::createD3d11Device() {
#ifdef HAS_NVENC
  D3D_FEATURE_LEVEL feature_levels[] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0
  };

  D3D_FEATURE_LEVEL selected_level;
  HRESULT hr = D3D11CreateDevice(
    nullptr,  // Default adapter
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,  // No software rasterizer
    0,  // No flags
    feature_levels,
    sizeof(feature_levels) / sizeof(D3D_FEATURE_LEVEL),
    D3D11_SDK_VERSION,
    reinterpret_cast<ID3D11Device**>(&d3d_device_),
    &selected_level,
    reinterpret_cast<ID3D11DeviceContext**>(&d3d_context_)
  );

  if (FAILED(hr)) {
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool NvencEncoderImpl::loadNvencApi() {
#ifdef HAS_NVENC
  // TODO: Load nvEncodeAPI64.dll dynamically
  // For now, assume it's linked statically
  return true;
#else
  return false;
#endif
}

void NvencEncoderImpl::cleanup() {
  std::lock_guard<std::mutex> lock(mutex_);

#ifdef HAS_NVENC
  // TODO: Release NVENC resources
  // 1. Unregister input buffer
  // 2. Destroy input buffer
  // 3. Destroy output buffer
  // 4. Close encode session
  // 5. Destroy NVENC instance

  // Release D3D11 resources
  if (d3d_context_ != nullptr) {
    reinterpret_cast<ID3D11DeviceContext*>(d3d_context_)->Release();
    d3d_context_ = nullptr;
  }

  if (d3d_device_ != nullptr) {
    reinterpret_cast<ID3D11Device*>(d3d_device_)->Release();
    d3d_device_ = nullptr;
  }
#endif

  encoder_ = nullptr;
  d3d_device_ = nullptr;
  d3d_context_ = nullptr;
  registered_ptr_ = nullptr;
  input_buffer_ = nullptr;
  output_buffer_ = nullptr;

  initialized_ = false;
}

bool NvencEncoderImpl::applyConfig(const std::string& config_json) {
  // TODO: Parse JSON config and apply NVENC parameters
  (void)config_json;
  return true;
}

} // namespace screensdk
