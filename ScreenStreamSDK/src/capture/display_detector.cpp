#include <windows.h>
#include <wingdi.h>
#include "screensdk/capture/display_detector.h"

namespace screensdk {

namespace {

std::string wstring_to_utf8(const std::wstring& wstr) {
  if (wstr.empty()) return "";
  int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
                                 nullptr, 0, nullptr, nullptr);
  std::string result(size - 1, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
                     &result[0], size, nullptr, nullptr);
  return result;
}

} // namespace

std::vector<DisplayInfo> DisplayDetector::getDisplays() const {
  std::vector<DisplayInfo> displays;

  DISPLAY_DEVICEA device;
  device.cb = sizeof(device);

  DWORD device_index = 0;
  while (EnumDisplayDevicesA(nullptr, device_index, &device, 0)) {
    DEVMODEA mode;
    mode.dmSize = sizeof(mode);
    mode.dmDriverExtra = 0;

    if (EnumDisplaySettingsA(device.DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
      DisplayInfo info;
      info.index = static_cast<int>(displays.size());
      info.name = device.DeviceName;
      info.width = mode.dmPelsWidth;
      info.height = mode.dmPelsHeight;
      info.refresh_rate = mode.dmDisplayFrequency;
      info.is_primary = (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;

      // Get desktop rectangle
      MONITORINFOEXA monitor_info;
      monitor_info.cbSize = sizeof(monitor_info);
      // Create a RECT from mode.dmPosition and size
      RECT rect = {
        mode.dmPosition.x,
        mode.dmPosition.y,
        mode.dmPosition.x + mode.dmPelsWidth,
        mode.dmPosition.y + mode.dmPelsHeight
      };
      HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
      if (GetMonitorInfoA(hMonitor, &monitor_info)) {
        info.desktop_rect = monitor_info.rcMonitor;
      } else {
        info.desktop_rect = {0, 0, info.width, info.height};
      }

      displays.push_back(info);
    }
    device_index++;
  }

  return displays;
}

DisplayInfo DisplayDetector::getPrimaryDisplay() const {
  auto displays = getDisplays();
  for (const auto& display : displays) {
    if (display.is_primary) {
      return display;
    }
  }
  return displays.empty() ? DisplayInfo{} : displays[0];
}

DisplayInfo DisplayDetector::getDisplay(int index) const {
  auto displays = getDisplays();
  if (index >= 0 && static_cast<size_t>(index) < displays.size()) {
    return displays[index];
  }
  return DisplayInfo{};
}

int DisplayDetector::getDisplayCount() const {
  return static_cast<int>(getDisplays().size());
}

bool DisplayDetector::hasDisplayChanged() const {
  int current_count = getDisplayCount();
  
  // If this is the first call, return false and initialize last_display_count_
  if (last_display_count_ == 0) {
    last_display_count_ = current_count;
    return false;
  }
  
  bool changed = (current_count != last_display_count_);
  last_display_count_ = current_count;
  return changed;
}

void DisplayDetector::refresh() {
  cached_displays_ = getDisplays();
  last_display_count_ = static_cast<int>(cached_displays_.size());
}

} // namespace screensdk
