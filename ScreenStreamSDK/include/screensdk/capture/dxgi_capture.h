#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <stop_token>

#include "screensdk/transport/video_source.h"
#include "screensdk/capture/display_detector.h"

namespace screensdk {

using Microsoft::WRL::ComPtr;

/**
 * @brief DXGI screen capture implementation
 * 
 * T019: Implement DXGI screen capture initialization
 * T037: Implement DXGI screen capture loop at 60fps
 * 
 * Captures screen content using Desktop Duplication API (DXGI 1.2+).
 * Provides high-performance capture with minimal CPU overhead.
 */
class DxgiCapture : public IVideoSource {
public:
  DxgiCapture();
  ~DxgiCapture() override;

  // IVideoSource interface
  void start() override;
  void stop() override;
  bool isRunning() const override { return running_; }
  void setFrameCallback(FrameCallback callback) override {
    frame_callback_ = callback;
  }
  void getFrameSize(int* width, int* height) const override;
  int getFps() const override { return target_fps_; }

  /**
   * @brief Initialize for specific display
   * @param display_index Display index to capture (0 = primary)
   */
  bool initialize(int display_index = 0);

  /**
   * @brief Capture single frame (blocking)
   */
  bool captureFrame(VideoFrame& frame);

  /**
   * @brief Set target FPS
   */
  void setTargetFps(int fps) { target_fps_ = fps; }

private:
  /**
   * @brief Capture loop thread function
   */
  void captureLoop(std::stop_token stop_token);

  /**
   * @brief Initialize DirectX 11 device and DXGI resources
   */
  bool initializeDxgi();

  /**
   * @brief Release DXGI resources
   */
  void releaseDxgi();

  // DirectX/DXGI resources
  ComPtr<ID3D11Device> d3d_device_;
  ComPtr<ID3D11DeviceContext> d3d_context_;
  ComPtr<IDXGIOutputDuplication> duplication_;
  ComPtr<ID3D11Texture2D> texture_copy_;

  // Capture state
  int display_index_{0};
  int width_{0};
  int height_{0};
  int target_fps_{60};
  std::atomic<bool> running_{false};
  FrameCallback frame_callback_;

  // Capture thread
  std::jthread capture_thread_;
};

} // namespace screensdk
