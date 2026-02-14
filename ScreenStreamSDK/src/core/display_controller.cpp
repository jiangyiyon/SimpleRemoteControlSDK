#include "screensdk/core/display_controller.h"

#include <algorithm>
#include <mutex>
#include <regex>
#include <unordered_map>

#include "screensdk/capture/display_detector.h"

namespace screensdk {

class DisplayControllerImpl : public IDisplayController {
public:
  DisplayControllerImpl() = default;
  ~DisplayControllerImpl() override;

  DisplayControllerImpl(const DisplayControllerImpl&) = delete;
  DisplayControllerImpl& operator=(const DisplayControllerImpl&) = delete;
  DisplayControllerImpl(DisplayControllerImpl&&) = delete;
  DisplayControllerImpl& operator=(DisplayControllerImpl&&) = delete;

  Result<void> initialize() override;
  void close() override;

  std::vector<DisplaySource> getDisplayList() override;
  DisplaySource getPrimaryDisplay() override;
  DisplaySource getDisplayById(int id) override;
  int getCurrentDisplayId() const noexcept override;
  DisplaySource getCurrentDisplay() override;

  Result<void> selectDisplay(int id) override;
  Result<void> switchDisplay(int id) override;

  bool detectDisplayChanges() override;
  void refreshDisplayList() override;

  void onDisplaySwitch(DisplaySwitchCallback callback) override;
  void onDisplayChange(DisplayChangeCallback callback) override;

  Result<void> selectDisplayForSession(const std::string& session_id,
                                       int id) override;
  Result<void> switchDisplayForSession(const std::string& session_id,
                                        int id) override;
  std::optional<DisplaySource> getDisplayForSession(
      const std::string& session_id) override;

private:
  std::vector<DisplaySource> convertToDisplaySources(
      const std::vector<DisplayInfo>& display_infos);
  DisplaySource convertToDisplaySource(const DisplayInfo& display_info);
  Result<void> validateDisplayId(int id);
  Result<void> validateSessionId(const std::string& session_id);
  void triggerDisplaySwitchCallback(const DisplaySource& old_display,
                                     const DisplaySource& new_display);

  DisplayDetector display_detector_;
  std::vector<DisplaySource> displays_;
  std::atomic<int> current_display_id_{-1};
  std::atomic<bool> initialized_{false};
  mutable std::mutex displays_mutex_;
  DisplaySwitchCallback display_switch_callback_;
  DisplayChangeCallback display_change_callback_;
  mutable std::mutex callback_mutex_;

  // Session-aware display selection
  std::unordered_map<std::string, int> session_display_map_;
  mutable std::mutex session_display_mutex_;
};

DisplayControllerImpl::~DisplayControllerImpl() {
  close();
}

Result<void> DisplayControllerImpl::initialize() {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  if (initialized_) {
    return Result<void>::make_ok();
  }

  // Refresh display list
  display_detector_.refresh();
  displays_ = convertToDisplaySources(display_detector_.getDisplays());

  if (displays_.empty()) {
    return Result<void>::make_error(ErrorType::kCaptureError, 0,
                                    "No displays detected");
  }

  initialized_ = true;
  return Result<void>::make_ok();
}

void DisplayControllerImpl::close() {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  displays_.clear();
  current_display_id_.store(-1);
  initialized_.store(false);
}

std::vector<DisplaySource> DisplayControllerImpl::getDisplayList() {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  if (!initialized_) {
    return {};
  }

  return displays_;
}

DisplaySource DisplayControllerImpl::getPrimaryDisplay() {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  if (!initialized_) {
    return DisplaySource{};
  }

  auto it = std::find_if(
      displays_.begin(), displays_.end(),
      [](const DisplaySource& display) { return display.is_primary; });

  if (it != displays_.end()) {
    return *it;
  }

  return DisplaySource{};
}

DisplaySource DisplayControllerImpl::getDisplayById(int id) {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  if (!initialized_) {
    return DisplaySource{};
  }

  auto it = std::find_if(displays_.begin(), displays_.end(),
                         [id](const DisplaySource& display) {
                           return display.id == id;
                         });

  if (it != displays_.end()) {
    return *it;
  }

  return DisplaySource{};
}

int DisplayControllerImpl::getCurrentDisplayId() const noexcept {
  return current_display_id_.load();
}

DisplaySource DisplayControllerImpl::getCurrentDisplay() {
  int current_id = current_display_id_.load();

  if (current_id == -1) {
    std::lock_guard<std::mutex> lock(displays_mutex_);
    
    if (!initialized_) {
      return DisplaySource{};
    }

    auto it = std::find_if(
        displays_.begin(), displays_.end(),
        [](const DisplaySource& display) { return display.is_primary; });

    if (it != displays_.end()) {
      return *it;
    }

    return DisplaySource{};
  }

  return getDisplayById(current_id);
}

Result<void> DisplayControllerImpl::selectDisplay(int id) {
  auto validation_result = validateDisplayId(id);
  if (!validation_result) {
    return validation_result;
  }

  current_display_id_.store(id);
  return Result<void>::make_ok();
}

Result<void> DisplayControllerImpl::switchDisplay(int id) {
  auto validation_result = validateDisplayId(id);
  if (!validation_result) {
    return validation_result;
  }

  int old_id = current_display_id_.load();
  DisplaySource old_display = getDisplayById(old_id);
  DisplaySource new_display = getDisplayById(id);

  current_display_id_.store(id);

  // Trigger display switch callback (will trigger SDP renegotiation)
  triggerDisplaySwitchCallback(old_display, new_display);

  return Result<void>::make_ok();
}

bool DisplayControllerImpl::detectDisplayChanges() {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  if (!initialized_) {
    return false;
  }

  bool has_changes = display_detector_.hasDisplayChanged();

  if (has_changes) {
    std::lock_guard<std::mutex> callback_lock(callback_mutex_);
    if (display_change_callback_) {
      display_change_callback_();
    }
  }

  return has_changes;
}

void DisplayControllerImpl::refreshDisplayList() {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  if (!initialized_) {
    return;
  }

  display_detector_.refresh();
  displays_ = convertToDisplaySources(display_detector_.getDisplays());
}

void DisplayControllerImpl::onDisplaySwitch(DisplaySwitchCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  display_switch_callback_ = std::move(callback);
}

void DisplayControllerImpl::onDisplayChange(DisplayChangeCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  display_change_callback_ = std::move(callback);
}

std::vector<DisplaySource> DisplayControllerImpl::convertToDisplaySources(
    const std::vector<DisplayInfo>& display_infos) {
  std::vector<DisplaySource> display_sources;

  for (const auto& display_info : display_infos) {
    DisplaySource display_source = convertToDisplaySource(display_info);
    display_sources.push_back(display_source);
  }

  return display_sources;
}

DisplaySource DisplayControllerImpl::convertToDisplaySource(
    const DisplayInfo& display_info) {
  DisplaySource display_source;
  display_source.id = display_info.index;
  display_source.name = display_info.name;
  display_source.resolution_width = display_info.width;
  display_source.resolution_height = display_info.height;
  display_source.refresh_rate = display_info.refresh_rate;
  display_source.is_primary = display_info.is_primary;
  display_source.is_active = true;
  display_source.capture_handle = nullptr;

  return display_source;
}

Result<void> DisplayControllerImpl::validateDisplayId(int id) {
  std::lock_guard<std::mutex> lock(displays_mutex_);

  if (!initialized_) {
    return Result<void>::make_error(ErrorType::kCaptureError, 0,
                                    "DisplayController not initialized");
  }

  auto it = std::find_if(displays_.begin(), displays_.end(),
                         [id](const DisplaySource& display) {
                           return display.id == id;
                         });

  if (it == displays_.end()) {
    return Result<void>::make_error(ErrorType::kInputError, 0,
                                    "Invalid display ID: " + std::to_string(id));
  }

  return Result<void>::make_ok();
}

void DisplayControllerImpl::triggerDisplaySwitchCallback(
    const DisplaySource& old_display, const DisplaySource& new_display) {
  std::lock_guard<std::mutex> lock(callback_mutex_);

  if (display_switch_callback_) {
    display_switch_callback_(old_display, new_display);
  }
}

Result<void> DisplayControllerImpl::validateSessionId(
    const std::string& session_id) {
  // UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  // where x is any hex digit (0-9, a-f), y is 8, 9, a, or b
  // Simplified validation: check length and format
  if (session_id.length() != 36) {
    return Result<void>::make_error(ErrorType::kInputError, 0,
                                    "Invalid session ID length");
  }

  // Check dash positions: 8, 13, 18, 23
  if (session_id[8] != '-' || session_id[13] != '-' ||
      session_id[18] != '-' || session_id[23] != '-') {
    return Result<void>::make_error(ErrorType::kInputError, 0,
                                    "Invalid session ID format (dashes)");
  }

  // Check version digit at position 14
  if (session_id[14] != '4') {
    return Result<void>::make_error(ErrorType::kInputError, 0,
                                    "Invalid session ID version");
  }

  // Check variant digit at position 19 (should be hex digit)
  // UUID v4 variant should be 10xx (8, 9, A, B), but we accept any hex digit
  char variant = session_id[19];
  bool is_variant_hex = (variant >= '0' && variant <= '9') ||
                        (variant >= 'A' && variant <= 'F') ||
                        (variant >= 'a' && variant <= 'f');
  if (!is_variant_hex) {
    return Result<void>::make_error(ErrorType::kInputError, 0,
                                    "Invalid session ID variant");
  }

  // Check remaining characters are hex digits
  for (int i = 0; i < 36; ++i) {
    if (i == 8 || i == 13 || i == 18 || i == 23) continue;
    char c = session_id[i];
    bool is_hex = (c >= '0' && c <= '9') ||
                  (c >= 'A' && c <= 'F') ||
                  (c >= 'a' && c <= 'f');
    if (!is_hex) {
      return Result<void>::make_error(ErrorType::kInputError, 0,
                                      "Invalid session ID format (non-hex)");
    }
  }

  return Result<void>::make_ok();
}

Result<void> DisplayControllerImpl::selectDisplayForSession(
    const std::string& session_id, int id) {
  auto session_validation = validateSessionId(session_id);
  if (!session_validation) {
    return session_validation;
  }

  auto display_validation = validateDisplayId(id);
  if (!display_validation) {
    return display_validation;
  }

  std::lock_guard<std::mutex> lock(session_display_mutex_);
  session_display_map_[session_id] = id;

  return Result<void>::make_ok();
}

Result<void> DisplayControllerImpl::switchDisplayForSession(
    const std::string& session_id, int id) {
  auto session_validation = validateSessionId(session_id);
  if (!session_validation) {
    return session_validation;
  }

  auto display_validation = validateDisplayId(id);
  if (!display_validation) {
    return display_validation;
  }

  std::lock_guard<std::mutex> lock(session_display_mutex_);

  int old_id = -1;
  auto it = session_display_map_.find(session_id);
  if (it != session_display_map_.end()) {
    old_id = it->second;
  }

  DisplaySource old_display = (old_id != -1) ? getDisplayById(old_id)
                                              : DisplaySource{};
  DisplaySource new_display = getDisplayById(id);

  session_display_map_[session_id] = id;

  // Trigger display switch callback (will trigger SDP renegotiation)
  triggerDisplaySwitchCallback(old_display, new_display);

  return Result<void>::make_ok();
}

std::optional<DisplaySource> DisplayControllerImpl::getDisplayForSession(
    const std::string& session_id) {
  auto session_validation = validateSessionId(session_id);
  if (!session_validation) {
    return std::nullopt;
  }

  std::lock_guard<std::mutex> lock(session_display_mutex_);

  // Check if session exists in map
  auto it = session_display_map_.find(session_id);
  if (it == session_display_map_.end()) {
    // Session not found in map
    return std::nullopt;
  }

  // Get display by ID from the map
  int display_id = it->second;
  DisplaySource display = getDisplayById(display_id);

  // Check if display is valid (empty id means not found)
  if (display.id < 0) {
    return std::nullopt;
  }

  return display;
}

// Factory function implementation
extern "C" SCREEN_STREAM_SDK_EXPORT IDisplayController*
CreateDisplayController() {
  return new DisplayControllerImpl();
}

extern "C" SCREEN_STREAM_SDK_EXPORT void
DestroyDisplayController(IDisplayController* controller) {
  if (controller != nullptr) {
    delete controller;
  }
}

} // namespace screensdk
