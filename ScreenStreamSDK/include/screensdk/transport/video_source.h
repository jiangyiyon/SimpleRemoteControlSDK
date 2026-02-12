#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "screensdk/export.h"

namespace screensdk {

/**
 * @brief Video frame data
 */
struct VideoFrame {
  uint8_t* data{nullptr};
  size_t size{0};
  int width{0};
  int height{0};
  int stride{0};  // Bytes per row (may be larger than width * 4 due to alignment)
  uint64_t timestamp_ms{0};
};

/**
 * @brief Frame callback for new video data
 */
using FrameCallback = std::function<void(const VideoFrame&)>;

/**
 * @brief Custom video source adapter for WebRTC track
 *
 * T016: Implement custom video source adapter for WebRTC track
 *
 * Adapts screen capture output to WebRTC video track format.
 * Handles frame format conversion and reference counting.
 */
struct IVideoSource {
  virtual ~IVideoSource() = default;

  /**
   * @brief Start video capture
   */
  virtual void start() = 0;

  /**
   * @brief Stop video capture
   */
  virtual void stop() = 0;

  /**
   * @brief Check if capture is active
   */
  virtual bool isRunning() const = 0;

  /**
   * @brief Set frame callback
   */
  virtual void setFrameCallback(FrameCallback callback) = 0;

  /**
   * @brief Get current frame dimensions
   */
  virtual void getFrameSize(int* width, int* height) const = 0;

  /**
   * @brief Get current FPS
   */
  virtual int getFps() const = 0;
};

/**
 * @brief Factory function to create video source
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IVideoSource* CreateVideoSource();
extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyVideoSource(IVideoSource* source);

} // namespace screensdk
