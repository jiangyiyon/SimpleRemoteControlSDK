# Source files
set(SOURCE_FILES
    # Utils
    utils/error.cpp
    utils/metrics_collector.cpp
    utils/thread_pool.cpp

    # Capture
    capture/dxgi_capture.cpp
    capture/display_detector.cpp

    # Encoding
    encoding/encoder_factory.cpp
    encoding/encoder_config.cpp
    encoding/x264_encoder.cpp

    # Transport
    transport/video_source.cpp

    # Input
    input/windows_input.cpp

    # Platform
    platform/gpu_detector.cpp
)

# Header files
set(HEADER_FILES
    # Utils
    ${CMAKE_SOURCE_DIR}/include/screensdk/utils/error.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/utils/metrics_collector.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/utils/thread_pool.h

    # Capture
    ${CMAKE_SOURCE_DIR}/include/screensdk/capture/dxgi_capture.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/capture/display_detector.h

    # Encoding
    ${CMAKE_SOURCE_DIR}/include/screensdk/encoding/encoder_factory.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/encoding/encoder_config.h
    ${CMAKE_SOURCE_DIR}/include/screensdk/encoding/x264_encoder.h

    # Transport
    ${CMAKE_SOURCE_DIR}/include/screensdk/transport/video_source.h

    # Input
    ${CMAKE_SOURCE_DIR}/include/screensdk/input/windows_input.h

    # Platform
    ${CMAKE_SOURCE_DIR}/include/screensdk/platform/gpu_detector.h
)

# Create library
add_library(screensdk STATIC ${SOURCE_FILES} ${HEADER_FILES})

# Link x264
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/x264/lib/x64/libx264.lib")
    target_link_libraries(screensdk
        ${CMAKE_SOURCE_DIR}/../third_party/x264/lib/x64/libx264.lib)
endif()

# Windows libraries
if(WIN32)
    target_link_libraries(screensdk d3d11 dxgi user32 gdi32)
endif()

# C++20
target_compile_features(screensdk PUBLIC cxx_std_20)

# Include directories
target_include_directories(screensdk PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    E:/TestWebRTC/RemoteControlSDK/ScreenStreamSDK/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/../third_party/x264/include
)

# Compile definitions
target_compile_definitions(screensdk PRIVATE
    SCREEN_STREAM_SDK_STATIC
)
