#pragma once

#include "screensdk/export.h"
#include "screensdk/capture/i_screen_capture.h"
#include "screensdk/transport/video_source.h"

namespace screensdk {

/**
 * @brief Factory function to create video source from screen capture
 *
 * Creates an IVideoSource adapter that wraps IScreenCapture.
 * Use this to bridge screen capture output to WebRTC video track.
 */
extern "C" SCREEN_STREAM_SDK_EXPORT IVideoSource* CreateVideoSourceFromScreenCapture(
    IScreenCapture* screen_capture);

} // namespace screensdk
