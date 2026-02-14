#include <windows.h>
#include <wingdi.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include "screensdk/capture/display_detector.h"

namespace screensdk {

using Microsoft::WRL::ComPtr;

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

std::vector<DisplaySource> DisplayDetector::getDisplaySources() const {
  std::vector<DisplaySource> sources;

  // Get display info from Windows Display API for primary display detection
  DISPLAY_DEVICEA device;
  device.cb = sizeof(device);
  int primary_adapter_index = -1;

  DWORD device_index = 0;
  while (EnumDisplayDevicesA(nullptr, device_index, &device, 0)) {
    if (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
      primary_adapter_index = device_index;
      break;
    }
    device_index++;
  }

  // Create DXGI Factory to enumerate adapters and outputs
  ComPtr<IDXGIFactory1> factory;
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), &factory);
  if (FAILED(hr)) {
    return sources;
  }

  // Enumerate adapters (up to 4 displays as per T058 constraint)
  UINT adapter_index = 0;
  ComPtr<IDXGIAdapter1> adapter;
  while (factory->EnumAdapters1(adapter_index, &adapter) != DXGI_ERROR_NOT_FOUND &&
         adapter_index < 4) {
    DXGI_ADAPTER_DESC1 adapter_desc;
    hr = adapter->GetDesc1(&adapter_desc);
    if (SUCCEEDED(hr)) {
      // Enumerate outputs (monitors) for this adapter
      UINT output_index = 0;
      ComPtr<IDXGIOutput> output;
      while (adapter->EnumOutputs(output_index, &output) != DXGI_ERROR_NOT_FOUND) {
        DXGI_OUTPUT_DESC output_desc;
        hr = output->GetDesc(&output_desc);
        if (SUCCEEDED(hr)) {
          // Get display mode list to get resolution and refresh rate
          UINT mode_count = 0;
          hr = output->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM,
                                           0, &mode_count, nullptr);
          if (SUCCEEDED(hr) && mode_count > 0) {
            std::vector<DXGI_MODE_DESC> modes(mode_count);
            hr = output->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM,
                                           0, &mode_count, modes.data());
            if (SUCCEEDED(hr) && !modes.empty()) {
              // Use the first mode (typically current mode)
              const DXGI_MODE_DESC& mode = modes[0];

              DisplaySource source;
              source.id = static_cast<int>(adapter_index) + 1;
              source.name = wstring_to_utf8(adapter_desc.Description);
              source.resolution_width = static_cast<int>(mode.Width);
              source.resolution_height = static_cast<int>(mode.Height);
              source.refresh_rate = static_cast<int>(
                  static_cast<float>(mode.RefreshRate.Numerator) /
                  static_cast<float>(mode.RefreshRate.Denominator));
              source.is_primary = (static_cast<int>(adapter_index) == primary_adapter_index);
              source.is_active = true;
              source.capture_handle = nullptr;

              sources.push_back(source);

              // Only process first output per adapter for now
              // TODO: Support multiple outputs per adapter
              break;
            }
          }
        }
        output_index++;
      }
    }
    adapter_index++;
  }

  return sources;
}

DisplaySource DisplayDetector::getDisplaySource(int id) const {
  auto sources = getDisplaySources();
  for (const auto& source : sources) {
    if (source.id == id) {
      return source;
    }
  }
  return DisplaySource{};
}

int DisplayDetector::getDisplaySourceCount() const {
  return static_cast<int>(getDisplaySources().size());
}

} // namespace screensdk
