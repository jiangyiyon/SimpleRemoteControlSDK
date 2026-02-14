#pragma once

#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "screensdk/capture/display_detector.h"
#include "screensdk/export.h"
#include "screensdk/utils/error.h"

namespace screensdk {

/**
 * @brief Display switch callback
 */
using DisplaySwitchCallback = std::function<void(const DisplaySource& old_display,
                                                   const DisplaySource& new_display)>;

/**
 * @brief Display configuration change callback
 */
using DisplayChangeCallback = std::function<void()>;

/**
 * @brief Display manager interface
 *
 * T059: Implement IDisplayManager interface
 *
 * Provides display enumeration, selection, and switching functions, supporting seamless
 * switching in multi-display scenarios. Integrates with SDP Renegotiation to achieve
 * ≤100ms switch time.
 */
struct SCREEN_STREAM_SDK_EXPORT IDisplayController {
  /**
 * @brief Virtual destructor to ensure derived class objects are properly destroyed
 */
virtual ~IDisplayController() = default;

  /**
   * @brief Initialize display controller
   * @return Success or failure
   */
  virtual Result<void> initialize() = 0;

  /**
   * @brief Close display controller
   */
  virtual void close() = 0;

  /**
   * @brief Get list of all available displays
   * @return Display list
   */
  virtual std::vector<DisplaySource> getDisplayList() = 0;

  /**
   * @brief Get primary display
   * @return Primary display information
   */
  virtual DisplaySource getPrimaryDisplay() = 0;

  /**
   * @brief Get display by ID
   * @param id Display ID
   * @return Display information, returns empty display if ID is invalid
   */
  virtual DisplaySource getDisplayById(int id) = 0;

  /**
   * @brief Get current selected display ID
   * @return Current display ID, returns -1 if not selected
   */
  virtual int getCurrentDisplayId() const noexcept = 0;

  /**
   * @brief Get current selected display
   * @return Current display information
   */
  virtual DisplaySource getCurrentDisplay() = 0;

  /**
   * @brief Select display (does not trigger SDP renegotiation)
   * @param id Display ID
   * @return Success or failure
   */
  virtual Result<void> selectDisplay(int id) = 0;

  /**
   * @brief Switch display (triggers SDP renegotiation)
   * @param id Target display ID
   * @return Success or failure
   */
  virtual Result<void> switchDisplay(int id) = 0;

  /**
   * @brief Detect display configuration changes
   * @return Whether display configuration has changed
   */
  virtual bool detectDisplayChanges() = 0;

  /**
   * @brief Refresh display list
   */
  virtual void refreshDisplayList() = 0;

  /**
   * @brief Set display switch callback
   * @param callback Callback function
   */
  virtual void onDisplaySwitch(DisplaySwitchCallback callback) = 0;

  /**
   * @brief Set display configuration change callback
   * @param callback Callback function
   */
  virtual void onDisplayChange(DisplayChangeCallback callback) = 0;

  /**
   * @brief Select display for specific session (does not trigger SDP renegotiation)
   * @param session_id Session ID
   * @param id Display ID
   * @return Success or failure
   *
   * T061: Implement display selection per session
   */
  virtual Result<void> selectDisplayForSession(const std::string& session_id,
                                               int id) = 0;

  /**
   * @brief Switch display for specific session (triggers SDP renegotiation)
   * @param session_id Session ID
   * @param id Target display ID
   * @return Success or failure
   *
   * T061: Implement display selection per session
   */
  virtual Result<void> switchDisplayForSession(const std::string& session_id,
                                                int id) = 0;

  /**
   * @brief Get current selected display for specific session
   * @param session_id Session ID
   * @return Session's current display information, returns empty if session doesn't exist or not selected
   *
   * T061: Implement display selection per session
   */
  virtual std::optional<DisplaySource> getDisplayForSession(
      const std::string& session_id) = 0;
};

/**
 * @brief Create DisplayController instance
 * @return DisplayController pointer
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IDisplayController* CreateDisplayController();

/**
 * @brief Destroy DisplayController instance
 * @param controller DisplayController pointer
 */
extern "C" SCREEN_STREAM_SDK_EXPORT void
DestroyDisplayController(IDisplayController* controller);

} // namespace screensdk
