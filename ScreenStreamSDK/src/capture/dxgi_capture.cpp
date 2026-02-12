#include "screensdk/capture/dxgi_capture.h"
#include <dxgi1_6.h>
#include <chrono>

namespace screensdk {

DxgiCapture::DxgiCapture() = default;

DxgiCapture::~DxgiCapture() {
  stop();
  releaseDxgi();
}

void DxgiCapture::start() {
  if (running_) return;

  if (!initializeDxgi()) {
    return;
  }

  running_ = true;
  capture_thread_ = std::jthread([](std::stop_token stop_token, DxgiCapture* capture) {
    capture->captureLoop(stop_token);
  }, this);
}

void DxgiCapture::stop() {
  running_ = false;
  capture_thread_.request_stop();
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
  // TODO: Implement frame capture using IDXGIOutputDuplication::AcquireNextFrame
  // This requires full DXGI desktop duplication implementation
  return false;
}

void DxgiCapture::captureLoop(std::stop_token stop_token) {
  const int frame_interval_ms = 1000 / target_fps_;

  while (!stop_token.stop_requested() && running_) {
    auto start = std::chrono::steady_clock::now();

    VideoFrame frame;
    if (captureFrame(frame) && frame_callback_) {
      frame_callback_(frame);
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
  // TODO: Full DXGI initialization
  // 1. Create D3D11 device
  // 2. Enumerate outputs and get IDXGIOutputDuplication
  // 3. Create texture for frame copy
  // 4. Query display dimensions

  return false;  // Placeholder
}

void DxgiCapture::releaseDxgi() {
  duplication_.Reset();
  texture_copy_.Reset();
  d3d_context_.Reset();
  d3d_device_.Reset();
}

} // namespace screensdk
