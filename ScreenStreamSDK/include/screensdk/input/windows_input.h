#pragma once

#include <cstdint>
#include <string>
#include <windows.h>

namespace screensdk {

/**
 * @brief Mouse button type
 */
enum class MouseButton {
  kLeft,
  kRight,
  kMiddle,
  kXButton1,
  kXButton2
};

/**
 * @brief Mouse event data
 */
struct MouseEvent {
  int x{0};
  int y{0};
  MouseButton button{MouseButton::kLeft};
  bool pressed{false};
  bool released{false};
  bool moved{false};
  int wheel_delta{0};
  uint64_t timestamp_ms{0};
};

/**
 * @brief Windows Input handler for mouse and keyboard
 * 
 * T023: Implement Windows SendInput wrapper for mouse events
 * T024: Implement Windows SendInput wrapper for keyboard events
 * 
 * Wraps Windows SendInput API for injecting input events.
 * Provides safe input injection with coordinate mapping.
 */
class WindowsInput {
public:
  WindowsInput() = default;
  ~WindowsInput() = default;

  // Disable copy and move
  WindowsInput(const WindowsInput&) = delete;
  WindowsInput& operator=(const WindowsInput&) = delete;
  WindowsInput(WindowsInput&&) = delete;
  WindowsInput& operator=(WindowsInput&&) = delete;

  /**
   * @brief Send mouse button event
   */
  bool sendMouseButton(const MouseEvent& event);

  /**
   * @brief Send mouse move event
   */
  bool sendMouseMove(int x, int y);

  /**
   * @brief Send mouse wheel event
   */
  bool sendMouseWheel(int delta);

  /**
   * @brief Send keyboard key down
   */
  bool sendKeyDown(int virtual_key);

  /**
   * @brief Send keyboard key up
   */
  bool sendKeyUp(int virtual_key);

  /**
   * @brief Send keyboard key press (down + up)
   */
  bool sendKeyPress(int virtual_key);

  /**
   * @brief Send text input (Unicode characters)
   */
  bool sendText(const std::string& text);

  /**
   * @brief Check if input injection is enabled
   */
  static bool isInputInjectionEnabled();

  /**
   * @brief Enable input injection (requires elevation)
   */
  static bool enableInputInjection();

private:
  /**
   * @brief Map virtual key to scan code
   */
  static WORD mapVirtualKeyToScanCode(int virtual_key);
};

} // namespace screensdk
