#include "screensdk/input/windows_input.h"
#include <windows.h>

namespace screensdk {

bool WindowsInput::sendMouseButton(const MouseEvent& event) {
  INPUT input = {};
  input.type = INPUT_MOUSE;

  DWORD mouse_data = 0;
  if (event.button == MouseButton::kLeft) {
    mouse_data = event.pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
  } else if (event.button == MouseButton::kRight) {
    mouse_data = event.pressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
  } else if (event.button == MouseButton::kMiddle) {
    mouse_data = event.pressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
  } else if (event.button == MouseButton::kXButton1) {
    mouse_data = event.pressed ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    input.mi.mouseData = XBUTTON1;
  } else if (event.button == MouseButton::kXButton2) {
    mouse_data = event.pressed ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    input.mi.mouseData = XBUTTON2;
  }

  input.mi.dwFlags = mouse_data;
  input.mi.dx = event.x;
  input.mi.dy = event.y;

  return SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool WindowsInput::sendMouseMove(int x, int y) {
  INPUT input = {};
  input.type = INPUT_MOUSE;
  input.mi.dx = x;
  input.mi.dy = y;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

  // Convert to absolute coordinates (0-65535 range)
  int screen_width = GetSystemMetrics(SM_CXSCREEN);
  int screen_height = GetSystemMetrics(SM_CYSCREEN);
  input.mi.dx = (x * 65535) / screen_width;
  input.mi.dy = (y * 65535) / screen_height;

  return SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool WindowsInput::sendMouseWheel(int delta) {
  INPUT input = {};
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = MOUSEEVENTF_WHEEL;
  input.mi.mouseData = delta;

  return SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool WindowsInput::sendKeyDown(int virtual_key) {
  INPUT input = {};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = static_cast<WORD>(virtual_key);
  input.ki.dwFlags = 0;

  return SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool WindowsInput::sendKeyUp(int virtual_key) {
  INPUT input = {};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = static_cast<WORD>(virtual_key);
  input.ki.dwFlags = KEYEVENTF_KEYUP;

  return SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool WindowsInput::sendKeyPress(int virtual_key) {
  return sendKeyDown(virtual_key) && sendKeyUp(virtual_key);
}

bool WindowsInput::sendText(const std::string& /*text*/) {
  // TODO: Implement Unicode text input via SendInput
  // Requires KEYEVENTF_UNICODE flag
  return false;
}

bool WindowsInput::isInputInjectionEnabled() {
  // Input injection requires UIAccess or elevated privileges
  // This is a simplified check
  return true;
}

bool WindowsInput::enableInputInjection() {
  // TODO: Implement elevation request for UIAccess
  // This requires manifest configuration and application signing
  return true;
}

WORD WindowsInput::mapVirtualKeyToScanCode(int virtual_key) {
  return static_cast<WORD>(MapVirtualKeyA(virtual_key, MAPVK_VK_TO_VSC));
}

} // namespace screensdk
