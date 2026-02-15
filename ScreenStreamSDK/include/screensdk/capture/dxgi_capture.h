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
  bool init() override;
  void uninit() override;
  void start() override;
  void stop() override;
  bool isRunning() const override { return running_; }
  void setFrameCallback(FrameCallback callback) override {
    frame_callback_ = callback;
  }
  void getFrameSize(int* width, int* height) const override;
  int getFps() const override { return target_fps_; }

  /**
   * @brief Select display index to capture
   * @param display_index Display index to capture (0 = primary)
   *
   * Must be called before init().
   */
  void selectDisplayIndex(int display_index);

  /**
   * @brief Capture single frame (blocking)
   */
  bool captureFrame(VideoFrameForTrans& frame);

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

  // Frame buffer for CPU readback
  std::vector<uint8_t> frame_buffer_;

  // Last frame buffer for maintaining frame rate when screen doesn't change
  std::vector<uint8_t> last_frame_buffer_;
  VideoFrameForTrans last_frame_;

  // Capture thread
  std::jthread capture_thread_;
};

} // namespace screensdk
