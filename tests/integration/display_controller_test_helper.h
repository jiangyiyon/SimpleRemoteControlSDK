#pragma once

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "screensdk/capture/display_detector.h"
#include "screensdk/core/display_controller.h"

namespace screensdk {

/**
 * @brief Test helper for display controller tests
 *
 * Provides utilities to test display controller behavior
 * without depending on actual hardware display count
 */
class DisplayControllerTestHelper {
public:
  /**
   * @brief Check if running with single display
   */
  static bool isSingleDisplayEnvironment(IDisplayController* controller) {
    auto displays = controller->getDisplayList();
    return displays.size() < 2;
  }

  /**
   * @brief Get mock displays for testing
   *
   * Creates a mock display list with 2 displays for testing
   * multi-display behavior on single-display systems
   */
  static std::vector<DisplaySource> getMockDisplays() {
    std::vector<DisplaySource> displays;

    DisplaySource primary;
    primary.id = 0;
    primary.name = "Mock Primary Display";
    primary.resolution_width = 1920;
    primary.resolution_height = 1080;
    primary.refresh_rate = 60;
    primary.is_primary = true;
    primary.is_active = true;
    primary.capture_handle = nullptr;
    displays.push_back(primary);

    DisplaySource secondary;
    secondary.id = 1;
    secondary.name = "Mock Secondary Display";
    secondary.resolution_width = 2560;
    secondary.resolution_height = 1440;
    secondary.refresh_rate = 60;
    secondary.is_primary = false;
    secondary.is_active = true;
    secondary.capture_handle = nullptr;
    displays.push_back(secondary);

    return displays;
  }

  /**
   * @brief Assert or skip based on display count
   *
   * If single display, skip the test with a message
   * If multiple displays, continue test execution
   */
  static void requireMultipleDisplays(IDisplayController* controller,
                                     const std::string& test_name) {
    if (isSingleDisplayEnvironment(controller)) {
      GTEST_SKIP() << test_name
                   << " requires at least 2 displays, only "
                   << controller->getDisplayList().size()
                   << " available. "
                   << "Use GTEST_SKIP to pass this test.";
    }
  }

  /**
   * @brief Log display information
   */
  static void logDisplays(IDisplayController* controller,
                          const std::string& prefix = "") {
    auto displays = controller->getDisplayList();

    if (!prefix.empty()) {
      std::cout << prefix << " ";
    }
    std::cout << "Found " << displays.size() << " display(s):" << std::endl;

    for (const auto& display : displays) {
      std::cout << "  [" << display.id << "] " << display.name << " ("
                << display.resolution_width << "x"
                << display.resolution_height << ") "
                << display.refresh_rate << "Hz"
                << (display.is_primary ? " [PRIMARY]" : "") << std::endl;
    }
  }

  /**
   * @brief Create a test scenario for display switch
   *
   * Returns pair of (from_display_id, to_display_id)
   * If single display, returns (primary_id, primary_id)
   */
  static std::pair<int, int> getDisplaySwitchScenario(
      IDisplayController* controller) {
    auto displays = controller->getDisplayList();

    if (displays.size() >= 2) {
      return {displays[0].id, displays[1].id};
    } else if (displays.size() == 1) {
      // Single display: use same display for both
      return {displays[0].id, displays[0].id};
    } else {
      // No displays: error case
      return {-1, -1};
    }
  }

  /**
   * @brief Run a test action if multiple displays available
   *
   * Returns true if action was executed, false if skipped
   */
  template <typename Action>
  static bool runIfMultipleDisplays(IDisplayController* controller,
                                   const std::string& test_name,
                                   Action action) {
    if (isSingleDisplayEnvironment(controller)) {
      std::cout << "[SKIP] " << test_name
                << " - Single display environment" << std::endl;
      return false;
    }

    action();
    return true;
  }
};

} // namespace screensdk
