#include <gtest/gtest.h>
#include "screensdk/input/windows_input.h"

using namespace screensdk;

class WindowsInputTest : public ::testing::Test {
protected:
  void SetUp() override {
    input_ = std::make_unique<WindowsInput>();
  }

  std::unique_ptr<WindowsInput> input_;
};

/**
 * @brief 测试发送鼠标左键按下事件
 * 
 * 验证 WindowsInput 类正确处理鼠标左键按下事件的能力
 * 
 * @param event 鼠标事件参数，包含:
 *   - button: 鼠标按钮类型 (此处为左键)
 *   - pressed: 按钮状态 (true表示按下)
 *   - x: 鼠标X坐标
 *   - y: 鼠标Y坐标
 * @return 验证发送操作是否成功返回true
 */
TEST_F(WindowsInputTest, DISABLED_SendMouseButtonLeftDown) {
  MouseEvent event;
  event.button = MouseButton::kLeft;
  event.pressed = true;
  event.x = 100;
  event.y = 200;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseButtonLeftUp) {
  MouseEvent event;
  event.button = MouseButton::kLeft;
  event.pressed = false;
  event.released = true;
  event.x = 100;
  event.y = 200;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseButtonRightDown) {
  MouseEvent event;
  event.button = MouseButton::kRight;
  event.pressed = true;
  event.x = 150;
  event.y = 250;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseButtonRightUp) {
  MouseEvent event;
  event.button = MouseButton::kRight;
  event.pressed = false;
  event.released = true;
  event.x = 150;
  event.y = 250;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseButtonMiddleDown) {
  MouseEvent event;
  event.button = MouseButton::kMiddle;
  event.pressed = true;
  event.x = 200;
  event.y = 300;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseButtonXButton1Down) {
  MouseEvent event;
  event.button = MouseButton::kXButton1;
  event.pressed = true;
  event.x = 250;
  event.y = 350;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseButtonXButton2Down) {
  MouseEvent event;
  event.button = MouseButton::kXButton2;
  event.pressed = true;
  event.x = 300;
  event.y = 400;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseMove) {
  int x = 500;
  int y = 500;

  bool result = input_->sendMouseMove(x, y);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseMoveOrigin) {
  int x = 0;
  int y = 0;

  bool result = input_->sendMouseMove(x, y);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseMoveLargeCoordinates) {
  int screen_width = GetSystemMetrics(SM_CXSCREEN);
  int screen_height = GetSystemMetrics(SM_CYSCREEN);

  bool result = input_->sendMouseMove(screen_width - 1, screen_height - 1);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseWheelPositive) {
  int delta = 120;

  bool result = input_->sendMouseWheel(delta);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseWheelNegative) {
  int delta = -120;

  bool result = input_->sendMouseWheel(delta);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseWheelZero) {
  int delta = 0;

  bool result = input_->sendMouseWheel(delta);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendMouseWheelLargeDelta) {
  int delta = 1200;

  bool result = input_->sendMouseWheel(delta);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendKeyDown) {
  int virtual_key = VK_SPACE;

  bool result = input_->sendKeyDown(virtual_key);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendKeyUp) {
  int virtual_key = VK_SPACE;

  bool result = input_->sendKeyUp(virtual_key);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendKeyPress) {
  int virtual_key = VK_RETURN;

  bool result = input_->sendKeyPress(virtual_key);

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendKeyPressMultipleKeys) {
  std::vector<int> keys = {VK_SPACE, VK_RETURN, VK_ESCAPE};

  for (auto key : keys) {
    bool result = input_->sendKeyPress(key);
    EXPECT_TRUE(result) << "Failed for key: " << key;
  }
}

TEST_F(WindowsInputTest, DISABLED_SendKeyDownUpSequence) {
  int virtual_key = 'A';

  bool down_result = input_->sendKeyDown(virtual_key);
  bool up_result = input_->sendKeyUp(virtual_key);

  EXPECT_TRUE(down_result);
  EXPECT_TRUE(up_result);
}

TEST_F(WindowsInputTest, DISABLED_SendTextNotImplemented) {
  std::string text = "Hello";

  bool result = input_->sendText(text);

  EXPECT_FALSE(result);
}

TEST_F(WindowsInputTest, DISABLED_SendTextEmptyString) {
  std::string text = "";

  bool result = input_->sendText(text);

  EXPECT_FALSE(result);
}

TEST_F(WindowsInputTest, DISABLED_IsInputInjectionEnabled) {
  bool enabled = WindowsInput::isInputInjectionEnabled();

  EXPECT_TRUE(enabled);
}

TEST_F(WindowsInputTest, DISABLED_EnableInputInjection) {
  bool result = WindowsInput::enableInputInjection();

  EXPECT_TRUE(result);
}

TEST_F(WindowsInputTest, DISABLED_MouseEventDefaultValues) {
  MouseEvent event;

  EXPECT_EQ(event.x, 0);
  EXPECT_EQ(event.y, 0);
  EXPECT_EQ(event.button, MouseButton::kLeft);
  EXPECT_FALSE(event.pressed);
  EXPECT_FALSE(event.released);
  EXPECT_FALSE(event.moved);
  EXPECT_EQ(event.wheel_delta, 0);
  EXPECT_EQ(event.timestamp_ms, 0);
}

TEST_F(WindowsInputTest, DISABLED_MouseEventCustomValues) {
  MouseEvent event;
  event.x = 123;
  event.y = 456;
  event.button = MouseButton::kRight;
  event.pressed = true;
  event.moved = true;
  event.wheel_delta = 120;
  event.timestamp_ms = 1234567890;

  EXPECT_EQ(event.x, 123);
  EXPECT_EQ(event.y, 456);
  EXPECT_EQ(event.button, MouseButton::kRight);
  EXPECT_TRUE(event.pressed);
  EXPECT_TRUE(event.moved);
  EXPECT_EQ(event.wheel_delta, 120);
  EXPECT_EQ(event.timestamp_ms, 1234567890);
}

TEST_F(WindowsInputTest, DISABLED_MouseButtonEnumValues) {
  EXPECT_EQ(static_cast<int>(MouseButton::kLeft), 0);
  EXPECT_EQ(static_cast<int>(MouseButton::kRight), 1);
  EXPECT_EQ(static_cast<int>(MouseButton::kMiddle), 2);
  EXPECT_EQ(static_cast<int>(MouseButton::kXButton1), 3);
  EXPECT_EQ(static_cast<int>(MouseButton::kXButton2), 4);
}

TEST_F(WindowsInputTest, DISABLED_MultipleMouseButtonClicks) {
  MouseEvent event;
  event.x = 100;
  event.y = 100;

  std::vector<MouseButton> buttons = {
    MouseButton::kLeft,
    MouseButton::kRight,
    MouseButton::kMiddle
  };

  for (auto button : buttons) {
    event.button = button;
    event.pressed = true;
    EXPECT_TRUE(input_->sendMouseButton(event)) << "Failed for button: " << static_cast<int>(button);

    event.pressed = false;
    event.released = true;
    EXPECT_TRUE(input_->sendMouseButton(event)) << "Failed for button: " << static_cast<int>(button);
  }
}

TEST_F(WindowsInputTest, DISABLED_SequenceMouseMoveAndClick) {
  int x = 300;
  int y = 400;

  EXPECT_TRUE(input_->sendMouseMove(x, y));

  MouseEvent event;
  event.x = x;
  event.y = y;
  event.button = MouseButton::kLeft;
  event.pressed = true;
  EXPECT_TRUE(input_->sendMouseButton(event));

  event.pressed = false;
  event.released = true;
  EXPECT_TRUE(input_->sendMouseButton(event));
}

TEST_F(WindowsInputTest, DISABLED_SendMultipleKeyPressesInSequence) {
  std::vector<int> keys = {'H', 'E', 'L', 'L', 'O'};

  for (auto key : keys) {
    EXPECT_TRUE(input_->sendKeyPress(key)) << "Failed for key: " << static_cast<char>(key);
  }
}

TEST_F(WindowsInputTest, DISABLED_SendMouseEventsWithDifferentCoordinates) {
  MouseEvent event;
  event.button = MouseButton::kLeft;
  event.pressed = true;

  std::vector<std::pair<int, int>> coordinates = {
    {0, 0},
    {100, 100},
    {500, 500},
    {1920, 1080}
  };

  for (auto [x, y] : coordinates) {
    event.x = x;
    event.y = y;
    EXPECT_TRUE(input_->sendMouseButton(event)) << "Failed at: (" << x << ", " << y << ")";
  }
}

TEST_F(WindowsInputTest, DISABLED_SendMouseWheelWithMultipleScrolls) {
  std::vector<int> deltas = {-360, -240, -120, 0, 120, 240, 360};

  for (auto delta : deltas) {
    EXPECT_TRUE(input_->sendMouseWheel(delta)) << "Failed for delta: " << delta;
  }
}

TEST_F(WindowsInputTest, DISABLED_MouseButtonReleasedAndMovedFlags) {
  MouseEvent event;
  event.x = 200;
  event.y = 200;
  event.button = MouseButton::kLeft;
  event.released = true;
  event.moved = true;

  bool result = input_->sendMouseButton(event);

  EXPECT_TRUE(result);
}
