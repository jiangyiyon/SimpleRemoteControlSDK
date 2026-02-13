#pragma once

#include <vector>

#include "screensdk/capture/display_detector.h"

namespace screensdk {

/**
 * @brief Mock DisplayDetector for testing
 *
 * Allows injecting fake display list for testing
 * display controller with single or multiple displays
 */
class MockDisplayDetector {
public:
  MockDisplayDetector() = default;
  ~MockDisplayDetector() = default;

  /**
   * @brief Set mock displays
   */
  void setDisplays(const std::vector<DisplayInfo>& displays) {
    displays_ = displays;
  }

  /**
   * @brief Get displays (returns mock data)
   */
  const std::vector<DisplayInfo>& getDisplays() const { return displays_; }

  /**
   * @brief Get display by index
   */
  DisplayInfo getDisplay(int index) const {
    for (const auto& display : displays_) {
      if (display.index == index) {
        return display;
      }
    }
    return DisplayInfo{};
  }

  /**
   * @brief Get primary display
   */
  DisplayInfo getPrimaryDisplay() const {
    for (const auto& display : displays_) {
      if (display.is_primary) {
        return display;
      }
    }
    return displays_.empty() ? DisplayInfo{} : displays_[0];
  }

  /**
   * @brief Refresh display list (no-op for mock)
   */
  void refresh() {}

  /**
   * @brief Check if display changed (returns false for mock)
   */
  bool hasDisplayChanged() const { return false; }

  /**
   * @brief Create a mock display list with specified count
   */
  static std::vector<DisplayInfo> createMockDisplays(int count) {
    std::vector<DisplayInfo> displays;

    for (int i = 0; i < count; ++i) {
      DisplayInfo display;
      display.index = i;
      display.name = "Mock Display " + std::to_string(i);
      display.width = 1920;
      display.height = 1080;
      display.refresh_rate = 60;
      display.is_primary = (i == 0);
      displays.push_back(display);
    }

    return displays;
  }

  /**
   * @brief Create mock displays with different resolutions
   */
  static std::vector<DisplayInfo> createMockDisplaysWithDifferentResolutions() {
    std::vector<DisplayInfo> displays;

    // Primary 1920x1080
    DisplayInfo display1;
    display1.index = 0;
    display1.name = "Mock Primary Display";
    display1.width = 1920;
    display1.height = 1080;
    display1.refresh_rate = 60;
    display1.is_primary = true;
    displays.push_back(display1);

    // Secondary 2560x1440
    DisplayInfo display2;
    display2.index = 1;
    display2.name = "Mock Secondary Display";
    display2.width = 2560;
    display2.height = 1440;
    display2.refresh_rate = 60;
    display2.is_primary = false;
    displays.push_back(display2);

    // Tertiary 1280x720
    DisplayInfo display3;
    display3.index = 2;
    display3.name = "Mock Tertiary Display";
    display3.width = 1280;
    display3.height = 720;
    display3.refresh_rate = 30;
    display3.is_primary = false;
    displays.push_back(display3);

    return displays;
  }

private:
  std::vector<DisplayInfo> displays_;
};

} // namespace screensdk
