#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <stop_token>
#include <atomic>
#include <mutex>

#include "screensdk/transport/video_source.h"
#include "screensdk/capture/display_detector.h"

namespace screensdk {

using Microsoft::WRL::ComPtr;

/**
 * @brief Error types for DXGI capture
 */
enum class CaptureError {
  kNone = 0,
  kAccessDenied,          // Session switch, lock screen
  kDeviceRemoved,         // GPU device lost
  kSessionDisconnected,   // RDP disconnected
  kInvalidCall,           // Internal state corrupted
  kUnknown
};

/**
 * @brief Error callback function type
 */
using ErrorCallback = std::function<void(const std::string&)>;

/**
 * @brief DXGI screen capture implementation
 *
 * T019: Implement DXGI screen capture initialization
 * T037: Implement DXGI screen capture loop at 60fps
 * T037 Phase 2: Error handling and recovery
 *
 * Captures screen content using Desktop Duplication API (DXGI 1.2+).
 * Provides high-performance capture with minimal CPU overhead.
 * Includes automatic error recovery for DXGI errors.
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

  /**
   * @brief Set error callback for critical errors
   * @param callback Function to call when critical error occurs
   */
  void setErrorCallback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
  }

private:
  /**
   * @brief Capture loop thread function
   */
  void captureLoop(std::stop_token stop_token);

  /**
   * @brief Recovery loop thread function
   */
  void recoveryLoop();

  /**
   * @brief Initialize DirectX 11 device and DXGI resources
   */
  bool initializeDxgi();

  /**
   * @brief Initialize DXGI resources without starting recovery thread
   * Used internally by recovery loop
   */
  bool initializeDxgiInternal();

  /**
   * @brief Release DXGI resources
   */
  void releaseDxgi();

  /**
   * @brief Classify DXGI error into error type
   * @param hr HRESULT from DXGI operation
   * @return Classified error type
   */
  CaptureError classifyError(HRESULT hr) const;

  /**
   * @brief Handle error and trigger recovery if needed
   * @param hr HRESULT from failed DXGI operation
   * @return true if recovery was triggered, false if error is fatal
   */
  bool handleError(HRESULT hr);

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

  // Error recovery
  std::thread recovery_thread_;
  std::atomic<bool> recovery_running_{false};
  std::atomic<bool> recovery_needed_{false};
  std::atomic<int> consecutive_failures_{0};
  static constexpr int kMaxConsecutiveFailures = 3;
  ErrorCallback error_callback_;

  // Frame buffer for CPU readback
  std::vector<uint8_t> frame_buffer_;

  // Last frame buffer for maintaining frame rate when screen doesn't change
  std::vector<uint8_t> last_frame_buffer_;
  VideoFrameForTrans last_frame_;

  // Capture thread
  std::jthread capture_thread_;
  mutable std::mutex capture_mutex_;  // Protect stop/uninit from concurrent calls
};

} // namespace screensdk
