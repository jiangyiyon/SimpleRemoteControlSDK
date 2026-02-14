#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "screensdk/capture/display_detector.h"
#include "screensdk/export.h"

namespace screensdk {

// Forward declarations (defined in video_source.h)
struct VideoFrame;

/**
 * @brief Resolution structure
 */
struct Resolution {
  int width{0};
  int height{0};
};

/**
 * @brief Pure virtual interface for screen capture
 *
 * I2: Contract File Implementation Mismatch - Alignment with contract
 *
 * Abstract screen capture interface supporting multiple displays with
 * hardware acceleration detection. Follows Pure Virtual Interface Pattern.
 *
 * Behavioral Guarantees:
 * - `getNextFrame()` must be thread-safe and non-blocking (timeout_ms = 0 for immediate return)
 * - Display enumeration must complete within 500ms
 * - Frame capture must maintain 60fps minimum (16.67ms per frame average)
 * - Memory allocation per frame ≤ 4MB (1080p@60fps worst case)
 * - Thread safety: `getNextFrame()` can be called from multiple threads
 *
 * Error Handling:
 * - Throws `std::runtime_error` if DXGI initialization fails
 * - Returns nullptr from `getNextFrame()` if capture stopped or timeout
 * - Calls error callback if duplicate output fails during capture
 */
struct SCREEN_STREAM_SDK_EXPORT IScreenCapture {
  virtual ~IScreenCapture() = default;

  // Lifecycle
  /**
   * @brief Initialize screen capture for specific display
   * @param display_id Display ID to capture (0 = primary)
   * @return true if initialization successful
   */
  virtual bool initialize(int display_id = 0) = 0;

  /**
   * @brief Shutdown and release resources
   */
  virtual void shutdown() = 0;

  // Display enumeration
  /**
   * @brief Enumerate all available displays
   * @return Vector of display sources (must complete within 500ms)
   */
  virtual std::vector<DisplaySource> enumerateDisplays() = 0;

  /**
   * @brief Get primary display
   * @return Primary display source
   */
  virtual DisplaySource getPrimaryDisplay() = 0;

  // Capture control
  /**
   * @brief Start capturing frames from specified display
   * @param display Display source to capture
   * @return true if capture started successfully
   */
  virtual bool startCapture(const DisplaySource& display) = 0;

  /**
   * @brief Stop capturing frames
   */
  virtual void stopCapture() = 0;

  /**
   * @brief Get currently captured display
   * @return Current display source (empty if not capturing)
   */
  virtual DisplaySource getCurrentDisplay() const = 0;

  // Frame acquisition (non-blocking)
  /**
   * @brief Get next captured frame (non-blocking)
   * @param timeout_ms Timeout in milliseconds (0 for immediate return)
   * @return Shared pointer to video frame, nullptr if timeout or stopped
   */
  virtual std::shared_ptr<VideoFrame> getNextFrame(uint32_t timeout_ms) = 0;

  // Capability queries
  /**
   * @brief Check if hardware encoding is supported
   * @return true if hardware encoding available (NVENC/QuickSync)
   */
  virtual bool supportsHardwareEncoding() const = 0;

  /**
   * @brief Get native resolution for display
   * @param display Display source
   * @return Resolution struct {width, height}
   */
  virtual Resolution getNativeResolution(const DisplaySource& display) const = 0;

  // Event callbacks
  /**
   * @brief Set callback for display configuration changes
   * @param callback Function called when display added/removed/resolution changes
   */
  virtual void setDisplayChangeCallback(std::function<void()> callback) = 0;

  /**
   * @brief Set callback for capture errors
   * @param callback Function called on capture errors with error message
   */
  virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};

// Factory functions for IScreenCapture
extern "C" SCREEN_STREAM_SDK_EXPORT IScreenCapture* CreateScreenCapture();
extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyScreenCapture(IScreenCapture* capture);

} // namespace screensdk
