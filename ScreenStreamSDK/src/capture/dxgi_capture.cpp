#include "screensdk/capture/dxgi_capture.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <chrono>
#include <algorithm>
#include <cstring>

namespace screensdk {

DxgiCapture::DxgiCapture() = default;

DxgiCapture::~DxgiCapture() {
  stop();
  releaseDxgi();
}

void DxgiCapture::start() {
  if (running_) return;

  // Only initialize if not already initialized
  if (!duplication_ && !initializeDxgi()) {
    return;
  }

  running_ = true;
  capture_thread_ = std::jthread([](std::stop_token stop_token, DxgiCapture* capture) {
    capture->captureLoop(stop_token);
  }, this);
}

void DxgiCapture::stop() {
  if (!running_) return;

  running_ = false;
  capture_thread_.request_stop();

  if (capture_thread_.joinable()) {
    capture_thread_.join();
  }
}

void DxgiCapture::getFrameSize(int* width, int* height) const {
  if (width) *width = width_;
  if (height) *height = height_;
}

bool DxgiCapture::initialize(int display_index) {
  display_index_ = display_index;
  return initializeDxgi();
}

bool DxgiCapture::captureFrame(VideoFrame& frame) {
  if (!duplication_ || !texture_copy_) {
    return false;
  }

  DXGI_OUTDUPL_FRAME_INFO frame_info;
  ComPtr<IDXGIResource> resource;
  HRESULT hr = duplication_->AcquireNextFrame(100, &frame_info, &resource);

  if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
    return false;
  }

  if (FAILED(hr)) {
    return false;
  }

  // RAII wrapper for ReleaseFrame
  auto release_frame_guard = [&]() {
    duplication_->ReleaseFrame();
  };

  ComPtr<ID3D11Texture2D> texture;
  hr = resource.As(&texture);
  if (FAILED(hr)) {
    release_frame_guard();
    return false;
  }

  D3D11_TEXTURE2D_DESC desc;
  texture->GetDesc(&desc);

  // Copy texture to CPU-accessible texture
  d3d_context_->CopyResource(texture_copy_.Get(), texture.Get());
  d3d_context_->Flush();

  // Map and copy to frame buffer
  D3D11_MAPPED_SUBRESOURCE mapped;
  hr = d3d_context_->Map(texture_copy_.Get(), 0, D3D11_MAP_READ, 0, &mapped);
  if (FAILED(hr)) {
    release_frame_guard();
    return false;
  }

  // RAII wrapper for Unmap
  auto unmap_guard = [&]() {
    d3d_context_->Unmap(texture_copy_.Get(), 0);
  };

  // Calculate buffer size with stride
  int src_stride = static_cast<int>(mapped.RowPitch);
  int dst_stride = src_stride;  // Use same stride for efficiency
  size_t buffer_size = dst_stride * static_cast<size_t>(desc.Height);

  // Create or resize frame buffer if needed
  if (!frame.data || frame.size != buffer_size) {
    frame_buffer_.resize(buffer_size);
    frame.data = frame_buffer_.data();
    frame.size = frame_buffer_.size();
  }

  frame.width = static_cast<int>(desc.Width);
  frame.height = static_cast<int>(desc.Height);
  frame.stride = dst_stride;
  frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();

  const uint8_t* src = static_cast<const uint8_t*>(mapped.pData);
  uint8_t* dst = frame.data;

  // Single memcpy for best performance
  std::memcpy(dst, src, frame.size);

  unmap_guard();
  release_frame_guard();

  return true;
}

void DxgiCapture::captureLoop(std::stop_token stop_token) {
  const int frame_interval_ms = 1000 / target_fps_;
  bool has_last_frame = false;

  while (!stop_token.stop_requested() && running_) {
    auto start = std::chrono::steady_clock::now();

    VideoFrame frame;
    bool captured = captureFrame(frame);

    if (captured && frame_callback_) {
      // Successfully captured new frame
      frame_callback_(frame);

      // Store last frame for reuse when screen doesn't change
      if (frame.size > 0 && frame.data) {
        last_frame_buffer_.resize(frame.size);
        std::memcpy(last_frame_buffer_.data(), frame.data, frame.size);

        last_frame_.data = last_frame_buffer_.data();
        last_frame_.size = last_frame_buffer_.size();
        last_frame_.width = frame.width;
        last_frame_.height = frame.height;
        last_frame_.stride = frame.stride;
        last_frame_.timestamp_ms = frame.timestamp_ms;
        has_last_frame = true;
      }
    } else if (has_last_frame && frame_callback_) {
      // Screen not changed, send last frame to maintain frame rate
      last_frame_.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch()).count();
      frame_callback_(last_frame_);
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto sleep_time = std::chrono::milliseconds(frame_interval_ms) -
                     std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    if (sleep_time.count() > 0) {
      std::this_thread::sleep_for(sleep_time);
    }
  }
}

bool DxgiCapture::initializeDxgi() {
  // Release any existing resources
  releaseDxgi();

  // Create D3D11 device
  D3D_FEATURE_LEVEL feature_levels[] = {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0
  };

  UINT device_flags = 0;
#ifdef _DEBUG
  device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  HRESULT hr = D3D11CreateDevice(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      device_flags,
      feature_levels,
      ARRAYSIZE(feature_levels),
      D3D11_SDK_VERSION,
      &d3d_device_,
      nullptr,
      &d3d_context_);

  if (FAILED(hr)) {
    return false;
  }

  // Get DXGI device
  ComPtr<IDXGIDevice> dxgi_device;
  hr = d3d_device_.As(&dxgi_device);
  if (FAILED(hr)) {
    return false;
  }

  // Get DXGI adapter
  ComPtr<IDXGIAdapter> dxgi_adapter;
  hr = dxgi_device->GetAdapter(&dxgi_adapter);
  if (FAILED(hr)) {
    return false;
  }

  // Enumerate outputs to find the target display
  ComPtr<IDXGIOutput> dxgi_output;
  UINT output_index = 0;

  while (dxgi_adapter->EnumOutputs(output_index, &dxgi_output) != DXGI_ERROR_NOT_FOUND) {
    if (output_index == static_cast<UINT>(display_index_)) {
      break;
    }
    output_index++;
    dxgi_output.Reset();
  }

  if (!dxgi_output) {
    return false;
  }

  // Get IDXGIOutput1 (required for Desktop Duplication)
  ComPtr<IDXGIOutput1> dxgi_output1;
  hr = dxgi_output.As(&dxgi_output1);
  if (FAILED(hr)) {
    return false;
  }

  // Create desktop duplication
  hr = dxgi_output1->DuplicateOutput(d3d_device_.Get(), &duplication_);
  if (FAILED(hr)) {
    return false;
  }

  // Get display description for dimensions
  DXGI_OUTPUT_DESC output_desc;
  dxgi_output->GetDesc(&output_desc);

  width_ = output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left;
  height_ = output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top;

  // Create texture for CPU readback
  D3D11_TEXTURE2D_DESC tex_desc = {};
  tex_desc.Width = static_cast<UINT>(width_);
  tex_desc.Height = static_cast<UINT>(height_);
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // Desktop Duplication always returns BGRA
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_STAGING;
  tex_desc.BindFlags = 0;
  tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  tex_desc.MiscFlags = 0;

  hr = d3d_device_->CreateTexture2D(&tex_desc, nullptr, &texture_copy_);
  if (FAILED(hr)) {
    duplication_.Reset();
    return false;
  }

  return true;
}

void DxgiCapture::releaseDxgi() {
  duplication_.Reset();
  texture_copy_.Reset();
  d3d_context_.Reset();
  d3d_device_.Reset();
  frame_buffer_.clear();
}

} // namespace screensdk
