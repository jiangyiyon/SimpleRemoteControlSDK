# Source files
set(SOURCE_FILES
    # API
    api/screensdk.cpp

    # Utils
    utils/error.cpp
    utils/metrics_collector.cpp
    utils/thread_pool.cpp

    # Capture
    capture/dxgi_capture.cpp
    capture/dxgi_capture_impl.cpp
    capture/display_detector.cpp
    capture/screen_capture_video_source.cpp

    # Encoding
    encoding/encoder_factory.cpp
    encoding/encoder_config.cpp
    encoding/x264_encoder.cpp
    encoding/nvenc_encoder.cpp

    # Transport
    transport/video_source.cpp
    transport/data_channel.cpp
    transport/sdp_renegotiation.cpp
    transport/webrtc_transport.cpp

    # Input
    input/windows_input.cpp
    # input/input_event.cpp  # TODO: Implement input_event.cpp

    # Core
    core/session.cpp
    # core/session_manager.cpp  # TODO: Implement session_manager.cpp
    core/display_controller.cpp

    # Capture
    # capture/capture_pipeline.cpp  # TODO: Implement capture_pipeline.cpp

    # Platform
    platform/gpu_detector.cpp

    # Server
    server/http_server.cpp
    server/signaling_server.cpp
    server/remote_desktop_server.cpp
)

# Header files
set(HEADER_FILES
    # API
    ${CMAKE_SOURCE_DIR}/include/screensdk/screensdk.h

    # Utils
    ${CMAKE_SOURCE_DIR}/include/screensdk/utils/error.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/utils/metrics_collector.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/utils/thread_pool.h

    # Capture
    ${CMAKE_SOURCE_DIR}/include/screensdk/capture/dxgi_capture.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/capture/i_screen_capture.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/capture/display_detector.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/capture/screen_capture_video_source.h

    # Encoding
    ${CMAKE_SOURCE_DIR}/include/screensdk/encoding/encoder_factory.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/encoding/encoder_config.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/encoding/x264_encoder.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/encoding/nvenc_encoder.h

    # Transport
    ${CMAKE_SOURCE_DIR}/include/screensdk/transport/video_source.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/transport/data_channel.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/transport/sdp_renegotiation.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/transport/i_webrtc_transport.h

    # Input
    ${CMAKE_SOURCE_DIR}/include/screensdk/input/windows_input.h
    # ${CMAKE_SOURCE_DIR}/include/screensdk/input/input_event.h  # TODO: Implement

    # Core
    ${CMAKE_SOURCE_DIR}/include/screensdk/core/session.h
    # ${CMAKE_SOURCE_DIR}/include/screensdk/core/session_manager.h  # TODO: Implement
    ${CMAKE_SOURCE_DIR}/include/screensdk/core/display_controller.h

    # Capture
    # ${CMAKE_SOURCE_DIR}/include/screensdk/capture/capture_pipeline.h  # TODO: Implement

    # Platform
    ${CMAKE_SOURCE_DIR}/include/screensdk/platform/gpu_detector.h

    # Server
    ${CMAKE_SOURCE_DIR}/include/screensdk/server/http_server.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/server/signaling_server.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/server/remote_desktop_server.h
)

# Create library
add_library(screensdk STATIC ${SOURCE_FILES} ${HEADER_FILES})

# Link x264
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/x264/lib/x64/libx264.lib")
    target_link_libraries(screensdk
        ${CMAKE_SOURCE_DIR}/../third_party/x264/lib/x64/libx264.lib)
endif()

# Link libdatachannel
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib")
        target_link_libraries(screensdk
            ${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib)
    endif()
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib")
        target_link_libraries(screensdk
            ${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib)
    endif()
endif()

# Windows libraries
if(WIN32)
    target_link_libraries(screensdk d3d11 dxgi d3d11.lib user32 gdi32)
endif()

# Link NVENC
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/nvenc/x64/nvencodeapi.lib")
    target_link_libraries(screensdk
        ${CMAKE_SOURCE_DIR}/../third_party/nvenc/x64/nvencodeapi.lib)
endif()

# C++20
target_compile_features(screensdk PUBLIC cxx_std_20)

# Include directories
target_include_directories(screensdk PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    E:/TestWebRTC/RemoteControlSDK/ScreenStreamSDK/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/../third_party/x264/include
    ${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows/include
)

# Compile definitions
target_compile_definitions(screensdk PRIVATE
    SCREEN_STREAM_SDK_STATIC
)
