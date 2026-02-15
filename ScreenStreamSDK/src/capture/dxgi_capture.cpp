#include "screensdk/capture/dxgi_capture.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <chrono>
#include <algorithm>
#include <cstring>

namespace screensdk {

DxgiCapture::DxgiCapture() = default;

DxgiCapture::~DxgiCapture() {
  uninit();
}

bool DxgiCapture::init() {
  // Ensure recovery thread is stopped before reinitializing
  recovery_running_ = false;
  if (recovery_thread_.joinable()) {
    recovery_thread_.join();
  }

  // Always reinitialize to support multiple init/uninit cycles
  bool result = initializeDxgiInternal();

  // Start recovery thread if initialization succeeded
  if (result) {
    recovery_running_ = true;
    recovery_thread_ = std::thread([this]() {
      recoveryLoop();
    });
  }

  return result;
}

void DxgiCapture::uninit() {
  std::lock_guard<std::mutex> lock(capture_mutex_);

  // Check if already stopped to avoid double cleanup
  if (!running_ && !capture_thread_.joinable()) {
    // Already uninitialized, just ensure recovery thread is stopped
    recovery_running_ = false;
    if (recovery_thread_.joinable()) {
      recovery_thread_.join();
    }
    releaseDxgi();
    return;
  }

  // Stop capture
  running_ = false;
  if (capture_thread_.joinable()) {
    capture_thread_.request_stop();
    capture_thread_.join();
  }

  // Stop recovery thread
  recovery_running_ = false;
  if (recovery_thread_.joinable()) {
    recovery_thread_.join();
  }

  releaseDxgi();
}

void DxgiCapture::start() {
  if (running_) return;

  if (!duplication_) {
    // Auto-init for backward compatibility
    if (!init()) {
      return;
    }
  }

  running_ = true;

  // Create new capture thread
  capture_thread_ = std::jthread([this](std::stop_token stop_token) {
    captureLoop(stop_token);
  });
}

void DxgiCapture::stop() {
  std::lock_guard<std::mutex> lock(capture_mutex_);

  if (!running_) return;

  running_ = false;
  capture_thread_.request_stop();

  if (capture_thread_.joinable()) {
    capture_thread_.join();
  }

  // Stop recovery thread
  recovery_running_ = false;
  if (recovery_thread_.joinable()) {
    recovery_thread_.join();
  }
}

void DxgiCapture::getFrameSize(int* width, int* height) const {
  if (width) *width = width_;
  if (height) *height = height_;
}

void DxgiCapture::selectDisplayIndex(int display_index) {
  display_index_ = display_index;
}

bool DxgiCapture::captureFrame(VideoFrameForTrans& frame) {
  if (!duplication_ || !texture_copy_) {
    return false;
  }

  // Use timeout based on target frame rate
  // For single blocking capture, use full frame interval to allow time for screen update
  const UINT kAcquireTimeoutMs = static_cast<UINT>(1000 / target_fps_);

  DXGI_OUTDUPL_FRAME_INFO frame_info;
  ComPtr<IDXGIResource> resource;
  HRESULT hr = duplication_->AcquireNextFrame(kAcquireTimeoutMs, &frame_info, &resource);

  if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
    return false;  // Normal timeout, not an error
  }

  if (FAILED(hr)) {
    // Handle error
    if (handleError(hr)) {
      return false;  // Recovery triggered, skip this frame
    } else {
      // Fatal error, stop capture
      running_ = false;
      return false;
    }
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
  const auto frame_interval = std::chrono::milliseconds(1000 / target_fps_);
  bool has_last_frame = false;
  auto next_frame_time = std::chrono::steady_clock::now();

  while (!stop_token.stop_requested() && running_) {
    // Calculate next frame time
    next_frame_time += frame_interval;
    auto now = std::chrono::steady_clock::now();

    // If we're behind schedule, skip this frame
    if (now > next_frame_time + std::chrono::milliseconds(10)) {
      next_frame_time = now;
      continue;
    }

    VideoFrameForTrans frame;
    bool captured = captureFrame(frame);

    if (captured && frame_callback_) {
      // Successfully captured new frame, reset failure counter
      consecutive_failures_ = 0;

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
    } else if (has_last_frame && frame_callback_ && !recovery_needed_) {
      // Screen not changed, send last frame to maintain frame rate
      // Only send last frame if recovery is not needed
      last_frame_.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch()).count();
      frame_callback_(last_frame_);
    }

    // Sleep until next frame time
    now = std::chrono::steady_clock::now();
    if (now < next_frame_time) {
      std::this_thread::sleep_until(next_frame_time);
    }
  }
}

bool DxgiCapture::initializeDxgi() {
  bool result = initializeDxgiInternal();

  // Start recovery thread if initialization succeeded
  if (result && !recovery_running_) {
    recovery_running_ = true;
    recovery_thread_ = std::thread([this]() {
      recoveryLoop();
    });
  }

  return result;
}

bool DxgiCapture::initializeDxgiInternal() {
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

CaptureError DxgiCapture::classifyError(HRESULT hr) const {
  switch (hr) {
    case DXGI_ERROR_ACCESS_DENIED:
      return CaptureError::kAccessDenied;
    case DXGI_ERROR_DEVICE_REMOVED:
      return CaptureError::kDeviceRemoved;
    case DXGI_ERROR_SESSION_DISCONNECTED:
      return CaptureError::kSessionDisconnected;
    case DXGI_ERROR_INVALID_CALL:
      return CaptureError::kInvalidCall;
    default:
      return CaptureError::kUnknown;
  }
}

bool DxgiCapture::handleError(HRESULT hr) {
  CaptureError error = classifyError(hr);

  // Check if max retries exceeded
  if (consecutive_failures_ >= kMaxConsecutiveFailures) {
    if (error_callback_) {
      std::string error_msg = "Max recovery attempts exceeded. Error: ";
      switch (error) {
        case CaptureError::kAccessDenied:
          error_msg += "DXGI_ERROR_ACCESS_DENIED";
          break;
        case CaptureError::kDeviceRemoved:
          error_msg += "DXGI_ERROR_DEVICE_REMOVED";
          break;
        case CaptureError::kSessionDisconnected:
          error_msg += "DXGI_ERROR_SESSION_DISCONNECTED";
          break;
        case CaptureError::kInvalidCall:
          error_msg += "DXGI_ERROR_INVALID_CALL";
          break;
        default:
          error_msg += "Unknown error";
          break;
      }
      error_callback_(error_msg);
    }
    return false;  // Fatal error, cannot recover
  }

  // Increment failure counter
  consecutive_failures_++;

  // Trigger recovery
  recovery_needed_ = true;
  return true;
}

void DxgiCapture::recoveryLoop() {
  while (recovery_running_) {
    // Wait for recovery request
    if (!recovery_needed_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    // Wait cooldown to avoid rapid retry
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Attempt recovery
    if (running_) {
      // Release existing resources
      releaseDxgi();

      // Reinitialize DXGI resources only (not the recovery thread)
      if (initializeDxgiInternal()) {
        // Recovery succeeded
        recovery_needed_ = false;
        consecutive_failures_ = 0;
      } else {
        // Recovery failed
        if (consecutive_failures_ >= kMaxConsecutiveFailures && error_callback_) {
          error_callback_("Recovery failed: cannot reinitialize DXGI");
        }
      }
    } else {
      // Capture is stopped, reset recovery flag
      recovery_needed_ = false;
    }
  }
}

} // namespace screensdk
