#pragma once

#include <Windows.h>
#include <string>
#include <vector>

namespace screensdk {

/**
 * @brief Display information
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
 * @brief Display detector for Windows Display API
 * 
 * T020: Implement display enumeration via Windows Display API
 * 
 * Provides display enumeration and display change notification.
 */
class DisplayDetector {
public:
  DisplayDetector() = default;
  ~DisplayDetector() = default;

  /**
   * @brief Get all available displays
   */
  std::vector<DisplayInfo> getDisplays() const;

  /**
   * @brief Get primary display info
   */
  DisplayInfo getPrimaryDisplay() const;

  /**
   * @brief Get display by index
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

private:
  mutable std::vector<DisplayInfo> cached_displays_;
  mutable int last_display_count_{0};
};

} // namespace screensdk
