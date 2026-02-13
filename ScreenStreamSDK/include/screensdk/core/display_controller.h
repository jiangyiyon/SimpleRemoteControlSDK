#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "screensdk/capture/display_detector.h"
#include "screensdk/export.h"
#include "screensdk/utils/error.h"

namespace screensdk {

/**
 * @brief 显示器切换回调
 */
using DisplaySwitchCallback = std::function<void(const DisplaySource& old_display,
                                                   const DisplaySource& new_display)>;

/**
 * @brief 显示器配置变化回调
 */
using DisplayChangeCallback = std::function<void()>;

/**
 * @brief 显示器管理器接口
 *
 * T059: Implement IDisplayManager interface
 *
 * 提供显示器枚举、选择和切换功能，支持多显示器场景下的无缝切换。
 * 与 SDP Renegotiation 集成实现≤100ms 切换时间。
 */
struct SCREEN_STREAM_SDK_EXPORT IDisplayController {
  virtual ~IDisplayController() = default;

  /**
   * @brief 初始化显示器控制器
   * @return 成功或失败
   */
  virtual Result<void> initialize() = 0;

  /**
   * @brief 关闭显示器控制器
   */
  virtual void close() = 0;

  /**
   * @brief 获取所有可用显示器列表
   * @return 显示器列表
   */
  virtual std::vector<DisplaySource> getDisplayList() = 0;

  /**
   * @brief 获取主显示器
   * @return 主显示器信息
   */
  virtual DisplaySource getPrimaryDisplay() = 0;

  /**
   * @brief 根据 ID 获取显示器
   * @param id 显示器 ID
   * @return 显示器信息，如果 ID 无效返回空显示器
   */
  virtual DisplaySource getDisplayById(int id) = 0;

  /**
   * @brief 获取当前选中的显示器 ID
   * @return 当前显示器 ID，未选择时返回 -1
   */
  virtual int getCurrentDisplayId() const noexcept = 0;

  /**
   * @brief 获取当前选中的显示器
   * @return 当前显示器信息
   */
  virtual DisplaySource getCurrentDisplay() = 0;

  /**
   * @brief 选择显示器 (不触发 SDP 重新协商)
   * @param id 显示器 ID
   * @return 成功或失败
   */
  virtual Result<void> selectDisplay(int id) = 0;

  /**
   * @brief 切换显示器 (触发 SDP 重新协商)
   * @param id 目标显示器 ID
   * @return 成功或失败
   */
  virtual Result<void> switchDisplay(int id) = 0;

  /**
   * @brief 检测显示器配置变化
   * @return 是否有显示器配置变化
   */
  virtual bool detectDisplayChanges() = 0;

  /**
   * @brief 刷新显示器列表
   */
  virtual void refreshDisplayList() = 0;

  /**
   * @brief 设置显示器切换回调
   * @param callback 回调函数
   */
  virtual void onDisplaySwitch(DisplaySwitchCallback callback) = 0;

  /**
   * @brief 设置显示器配置变化回调
   * @param callback 回调函数
   */
  virtual void onDisplayChange(DisplayChangeCallback callback) = 0;
};

/**
 * @brief 创建 DisplayController 实例
 * @return DisplayController 指针
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IDisplayController* CreateDisplayController();

/**
 * @brief 销毁 DisplayController 实例
 * @param controller DisplayController 指针
 */
extern "C" SCREEN_STREAM_SDK_EXPORT void
DestroyDisplayController(IDisplayController* controller);

} // namespace screensdk
