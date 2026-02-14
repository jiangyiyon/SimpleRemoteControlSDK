#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace screensdk {

/**
 * @brief Display information (legacy, use DisplaySource for new code)
 */
struct DisplayInfo {
  int index;
  std::string name;
  int width;
  int height;
  int refresh_rate;
  bool is_primary;
  RECT desktop_rect;
};

/**
 * @brief Display source for screen capture
 *
 * T058: Implement DisplaySource data structure
 *
 * Represents a Windows display device that can be captured and transmitted.
 */
struct DisplaySource {
  int id;
  std::string name;
  int resolution_width;
  int resolution_height;
  int refresh_rate;
  bool is_primary;
  bool is_active;
  void* capture_handle;

  DisplaySource()
      : id(0),
        name(),
        resolution_width(0),
        resolution_height(0),
        refresh_rate(0),
        is_primary(false),
        is_active(false),
        capture_handle(nullptr) {}
};

/**
 * @brief Display detector for Windows Display API
 *
 * T020: Implement display enumeration via Windows Display API
 * T060: Implement display enumeration from DXGI
 *
 * Provides display enumeration and display change notification.
 */
class DisplayDetector {
public:
  DisplayDetector() = default;
  ~DisplayDetector() = default;

  /**
   * @brief Get all available displays (legacy DisplayInfo)
   */
  std::vector<DisplayInfo> getDisplays() const;

  /**
   * @brief Get primary display info (legacy DisplayInfo)
   */
  DisplayInfo getPrimaryDisplay() const;

  /**
   * @brief Get display by index (legacy DisplayInfo)
   */
  DisplayInfo getDisplay(int index) const;

  /**
   * @brief Get display count
   */
  int getDisplayCount() const;

  /**
   * @brief Check for display changes since last check
   */
  bool hasDisplayChanged() const;

  /**
   * @brief Refresh display cache
   */
  void refresh();

  /**
   * @brief Get all available display sources using DXGI
   *
   * T060: Implement display enumeration from DXGI
   *
   * Returns DisplaySource list with DXGI-based display enumeration.
   * Each source contains id, name, resolution, refresh rate, and primary flag.
   */
  std::vector<DisplaySource> getDisplaySources() const;

  /**
   * @brief Get display source by ID using DXGI
   *
   * Returns DisplaySource for the specified ID (0-3).
   * Returns empty DisplaySource (id=0, name="") if ID is invalid.
   */
  DisplaySource getDisplaySource(int id) const;

  /**
   * @brief Get display source count
   */
  int getDisplaySourceCount() const;

private:
  mutable std::vector<DisplayInfo> cached_displays_;
  mutable int last_display_count_{0};
};

} // namespace screensdk
