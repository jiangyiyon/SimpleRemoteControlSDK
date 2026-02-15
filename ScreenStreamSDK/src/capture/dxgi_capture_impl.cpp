#include "screensdk/capture/i_screen_capture.h"
#include "screensdk/capture/dxgi_capture.h"
#include "screensdk/capture/display_detector.h"
#include "screensdk/transport/video_source.h"
#include <stdexcept>
#include <memory>
#include <mutex>
#include <cstring>

namespace screensdk {

/**
 * @brief Implementation of IScreenCapture interface
 *
 * Wraps existing DxgiCapture class to conform to IScreenCapture contract.
 * Follows Pure Virtual Interface Pattern with C-style factory functions.
 */
class DxgiCaptureImpl : public IScreenCapture {
public:
  DxgiCaptureImpl() = default;
  ~DxgiCaptureImpl() override {
    shutdown();
  }

  // Disable copy/move operations
  DxgiCaptureImpl(const DxgiCaptureImpl&) = delete;
  DxgiCaptureImpl& operator=(const DxgiCaptureImpl&) = delete;
  DxgiCaptureImpl(DxgiCaptureImpl&&) = delete;
  DxgiCaptureImpl& operator=(DxgiCaptureImpl&&) = delete;

  // IScreenCapture interface implementation

  bool initialize(int display_id) override {
    if (display_id < 1 || display_id > display_detector_.getDisplaySourceCount()) {
      return false;
    }

    if (!dxgi_capture_) {
      dxgi_capture_ = std::make_unique<DxgiCapture>();
    }

    dxgi_capture_->selectDisplayIndex(display_id - 1);
    return dxgi_capture_->init();
  }

  void shutdown() override {
    if (dxgi_capture_) {
      dxgi_capture_->stop();
      dxgi_capture_.reset();
    }
    current_display_ = DisplaySource();
  }

  std::vector<DisplaySource> enumerateDisplays() override {
    return display_detector_.getDisplaySources();
  }

  DisplaySource getPrimaryDisplay() override {
    auto displays = enumerateDisplays();
    for (const auto& display : displays) {
      if (display.is_primary) {
        return display;
      }
    }
    return DisplaySource();
  }

  bool startCapture(const DisplaySource& display) override {
    if (!dxgi_capture_) {
      return false;
    }

    current_display_ = display;
    dxgi_capture_->setFrameCallback([this](const VideoFrameForTrans& frame) {
      this->onFrameCaptured(frame);
    });
    dxgi_capture_->start();
    return true;
  }

  void stopCapture() override {
    if (dxgi_capture_) {
      dxgi_capture_->stop();
    }
  }

  DisplaySource getCurrentDisplay() const override {
    return current_display_;
  }

  std::shared_ptr<VideoFrameForTrans> getNextFrame(uint32_t timeout_ms) override {
    // Note: DxgiCapture uses callback-based model, not pull-based
    // This implementation returns the latest captured frame
    // For true non-blocking behavior, timeout_ms is ignored
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return latest_frame_;
  }

  bool supportsHardwareEncoding() const override {
    // TODO: Implement GPU capability detection (T026)
    // For now, return true as placeholder
    return true;
  }

  Resolution getNativeResolution(const DisplaySource& display) const override {
    return Resolution{
        display.resolution_width,
        display.resolution_height
    };
  }

  void setDisplayChangeCallback(std::function<void()> callback) override {
    display_change_callback_ = callback;
    // TODO: Integrate with DisplayDetector for change notification
  }

  void setErrorCallback(std::function<void(const std::string&)> callback) override {
    error_callback_ = callback;
  }

private:
  /**
   * @brief Frame captured callback from DxgiCapture
   */
  void onFrameCaptured(const VideoFrameForTrans& frame) {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    // Create a copy of the frame data
    latest_frame_ = std::make_shared<VideoFrameForTrans>();
    latest_frame_->width = frame.width;
    latest_frame_->height = frame.height;
    latest_frame_->stride = frame.stride;
    latest_frame_->timestamp_ms = frame.timestamp_ms;

    if (frame.size > 0 && frame.data) {
      latest_frame_->size = frame.size;
      frame_buffer_.resize(frame.size);
      std::memcpy(frame_buffer_.data(), frame.data, frame.size);
      latest_frame_->data = frame_buffer_.data();
    }
  }

  std::unique_ptr<DxgiCapture> dxgi_capture_;
  DisplayDetector display_detector_;
  DisplaySource current_display_;
  std::function<void()> display_change_callback_;
  std::function<void(const std::string&)> error_callback_;

  // Frame storage for getNextFrame()
  mutable std::mutex frame_mutex_;
  std::shared_ptr<VideoFrameForTrans> latest_frame_;
  std::vector<uint8_t> frame_buffer_;
};

// Factory functions

extern "C" SCREEN_STREAM_SDK_EXPORT IScreenCapture* CreateScreenCapture() {
  return new DxgiCaptureImpl();
}

extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyScreenCapture(IScreenCapture* capture) {
  if (capture != nullptr) {
    delete capture;
  }
}

} // namespace screensdk
