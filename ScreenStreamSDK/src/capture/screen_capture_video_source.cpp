/**
 * @file screen_capture_video_source.cpp
 * @brief Video source adapter for screen capture
 *
 * T017: Implement video source adapter for WebRTC track
 *
 * Adapts IScreenCapture output to IVideoSource interface.
 * Handles frame format conversion from RGB to YUV and manages
 * frame lifecycle for WebRTC transport.
 */

#include "screensdk/capture/i_screen_capture.h"
#include "screensdk/transport/video_source.h"
#include <iostream>
#include <mutex>
#include <chrono>

namespace screensdk {

namespace {

/**
 * @brief Video source adapter implementation
 *
 * Wraps IScreenCapture and provides IVideoSource interface
 * for WebRTC video track.
 */
class ScreenCaptureVideoSource : public IVideoSource {
public:
    ScreenCaptureVideoSource(IScreenCapture* screen_capture);
    ~ScreenCaptureVideoSource() override;

    ScreenCaptureVideoSource(const ScreenCaptureVideoSource&) = delete;
    ScreenCaptureVideoSource& operator=(const ScreenCaptureVideoSource&) = delete;
    ScreenCaptureVideoSource(ScreenCaptureVideoSource&&) = delete;
    ScreenCaptureVideoSource& operator=(ScreenCaptureVideoSource&&) = delete;

    // IVideoSource interface
    bool init() override;
    void uninit() override;
    void start() override;
    void stop() override;
    bool isRunning() const override;
    void setFrameCallback(FrameCallback callback) override;
    void getFrameSize(int* width, int* height) const override;
    int getFps() const override;

private:
    /**
     * @brief Capture thread loop
     */
    void captureThread();



    IScreenCapture* screen_capture_;
    FrameCallback frame_callback_;

    std::atomic<bool> running_{false};
    std::atomic<bool> capturing_{false};
    std::thread capture_thread_;

    std::mutex mutex_;

    int fps_ = 30;
    int target_width_ = 0;
    int target_height_ = 0;
};

ScreenCaptureVideoSource::ScreenCaptureVideoSource(IScreenCapture* screen_capture)
    : screen_capture_(screen_capture) {
    if (screen_capture_) {
        // Get display size from screen capture
        auto displays = screen_capture_->enumerateDisplays();
        if (!displays.empty()) {
            target_width_ = displays[0].resolution_width;
            target_height_ = displays[0].resolution_height;
        }
    }
}

ScreenCaptureVideoSource::~ScreenCaptureVideoSource() {
    stop();
    uninit();
}

bool ScreenCaptureVideoSource::init() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!screen_capture_) {
        return false;
    }

    std::cout << "[ScreenCaptureVideoSource] Initialized, frame size: "
              << target_width_ << "x" << target_height_ << std::endl;

    return true;
}

void ScreenCaptureVideoSource::uninit() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[ScreenCaptureVideoSource] Uninitialized" << std::endl;
}

void ScreenCaptureVideoSource::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_.load()) {
        return;
    }

    std::cout << "[ScreenCaptureVideoSource] Starting capture thread..." << std::endl;

    running_.store(true);
    capturing_.store(true);
    capture_thread_ = std::thread([this]() {
        captureThread();
    });
}

void ScreenCaptureVideoSource::stop() {
    std::cout << "[ScreenCaptureVideoSource] Stopping capture thread..." << std::endl;

    running_.store(false);

    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }

    std::cout << "[ScreenCaptureVideoSource] Capture thread stopped" << std::endl;
}

bool ScreenCaptureVideoSource::isRunning() const {
    return running_.load();
}

void ScreenCaptureVideoSource::setFrameCallback(FrameCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_callback_ = std::move(callback);
}

void ScreenCaptureVideoSource::getFrameSize(int* width, int* height) const {
    if (width) {
        *width = target_width_;
    }
    if (height) {
        *height = target_height_;
    }
}

int ScreenCaptureVideoSource::getFps() const {
    return fps_;
}

void ScreenCaptureVideoSource::captureThread() {
    std::cout << "[ScreenCaptureVideoSource] Capture thread started" << std::endl;

    // Calculate frame interval
    const auto frame_interval = std::chrono::milliseconds(1000 / fps_);
    auto next_frame_time = std::chrono::steady_clock::now();

    while (running_.load()) {
        // Wait until next frame time
        std::this_thread::sleep_until(next_frame_time);
        next_frame_time += frame_interval;

        if (!capturing_.load()) {
            continue;
        }

        // Get frame from screen capture
        auto frame = screen_capture_->getNextFrame(0);
        if (!frame || !frame->data) {
            continue;
        }

        // Send BGRA frame directly (x264 encoder supports BGRA format)
        std::lock_guard<std::mutex> lock(mutex_);
        if (frame_callback_) {
            VideoFrameForTrans trans_frame;
            trans_frame.data = frame->data;
            trans_frame.size = frame->width * frame->height * 4;
            trans_frame.width = target_width_;
            trans_frame.height = target_height_;
            trans_frame.stride = frame->width * 4;

            auto now = std::chrono::system_clock::now();
            auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            trans_frame.timestamp_ms = timestamp_ms;

            frame_callback_(trans_frame);
        }
    }

    std::cout << "[ScreenCaptureVideoSource] Capture thread ended" << std::endl;
}



} // anonymous namespace

// Factory functions
extern "C" SCREEN_STREAM_SDK_EXPORT IVideoSource* CreateVideoSource() {
    std::cerr << "[CreateVideoSource] Factory function called - requires IScreenCapture*" << std::endl;
    return nullptr;
}

extern "C" SCREEN_STREAM_SDK_EXPORT void DestroyVideoSource(IVideoSource* source) {
    if (source != nullptr) {
        delete source;
    }
}

// Special factory that creates adapter from screen capture
extern "C" SCREEN_STREAM_SDK_EXPORT IVideoSource* CreateVideoSourceFromScreenCapture(
    IScreenCapture* screen_capture) {
    if (!screen_capture) {
        std::cerr << "[CreateVideoSourceFromScreenCapture] screen_capture is null" << std::endl;
        return nullptr;
    }

    std::cout << "[CreateVideoSourceFromScreenCapture] Creating video source from screen capture"
              << std::endl;

    return new ScreenCaptureVideoSource(screen_capture);
}

} // namespace screensdk
