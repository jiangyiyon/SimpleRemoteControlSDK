cmake_minimum_required(VERSION 3.15)

# First try to use local GoogleTest from third_party
set(GTEST_FOUND FALSE)

# Check if local GoogleTest exists
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/googletest/CMakeLists.txt")
    # Case 1: Using main third_party directory
    set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/../third_party/googletest")
    message(STATUS "Using local GoogleTest: ${GTEST_ROOT}")
    add_subdirectory(${GTEST_ROOT} ${CMAKE_BINARY_DIR}/googletest)
    set(GTEST_FOUND TRUE)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party/googletest/CMakeLists.txt")
    # Case 2: Using local third_party directory
    set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/third_party/googletest")
    message(STATUS "Using local GoogleTest: ${GTEST_ROOT}")
    add_subdirectory(${GTEST_ROOT} ${CMAKE_BINARY_DIR}/googletest)
    set(GTEST_FOUND TRUE)
else()
    # Try to find GoogleTest using find_package
    message(STATUS "Looking for GoogleTest using find_package")
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Found system GoogleTest: ${GTest_DIR}")
        set(GTEST_FOUND TRUE)
    else()
        message(WARNING "GoogleTest not found. Please ensure googletest is properly installed in third_party/googletest.")
        return()
    endif()
endif()

if(GTEST_FOUND)
    # Create test executable
    enable_testing()
else()
    message(WARNING "GoogleTest not found, skipping tests")
    return()
endif()

add_executable(unit_tests
    # Unit tests for API
    unit/screensdk_api_test.cpp

    # Unit tests for utils
    unit/error_test.cpp
    unit/metrics_collector_test.cpp
    unit/thread_pool_test.cpp

    # Unit tests for capture
    unit/display_detector_test.cpp
    unit/screen_capture_test.cpp
    unit/capture/dxgi_capture_test.cpp
    unit/capture/dxgi_capture_error_test.cpp
    unit/capture/display_detector_dxgi_test.cpp
    # unit/capture/display_switch_test.cpp  # TODO: Update to use init/uninit

    # Unit tests for encoding
    unit/encoder_config_test.cpp
    unit/encoder_factory_test.cpp
    unit/encoding/x264_encoder_test.cpp
    unit/encoding/nvenc_encoder_test.cpp
    unit/encoding/encoding_fallback_test.cpp

    # Unit tests for core components
    unit/core/session_test.cpp
    unit/core/session_manager_test.cpp
    unit/core/display_controller_test.cpp
    unit/core/session_display_test.cpp

    # Unit tests for input handling
    unit/input/input_event_test.cpp
    unit/input/gesture_handler_test.cpp
    unit/input/windows_input_test.cpp

    # Unit tests for encoding
    unit/encoding/video_frame_test.cpp

    # Unit tests for platform
    unit/platform/gpu_detector_test.cpp

    # Unit tests for transport
    unit/transport/webrtc_manager_test.cpp
    unit/transport/connection_handler_test.cpp
    unit/transport/data_channel_test.cpp
    unit/transport/sdp_renegotiation_test.cpp
    unit/transport/transport_test.cpp
)

# Link Google Test
if(TARGET gtest_main AND TARGET gtest)
    # Using local GoogleTest
    target_link_libraries(unit_tests
        PRIVATE
        gtest_main
        gtest
    )
elseif(TARGET GTest::gtest_main AND TARGET GTest::gtest)
    # Using system GoogleTest
    target_link_libraries(unit_tests
        PRIVATE
        GTest::gtest_main
        GTest::gtest
    )
endif()

# Link project libraries
target_link_libraries(unit_tests
    PRIVATE
    screensdk
    ws2_32
    crypt32
    secur32
)

# Link libdatachannel
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib")
        target_link_libraries(unit_tests
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET unit_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/bin/*.dll
                $<TARGET_FILE_DIR:unit_tests>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib")
        target_link_libraries(unit_tests
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET unit_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/bin/*.dll
                $<TARGET_FILE_DIR:unit_tests>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
endif()

# Add libdatachannel include directory
target_include_directories(unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/include
)

# Add compile definition for static library
target_compile_definitions(unit_tests PRIVATE
    SCREEN_STREAM_SDK_STATIC
)

# Include test headers
target_include_directories(unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../ScreenStreamSDK/include
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/third_party/x264/include
)

# Register tests with CTest
include(GoogleTest)
# gtest_discover_tests(unit_tests)  # Temporarily disabled for linking

# Integration tests
add_executable(integration_tests
    integration/capture_encoder_test.cpp
    integration/capture_encoding_test.cpp
    integration/display_switch_integration_test.cpp
    integration/encoding_fallback_test.cpp
    integration/display_controller_sdp_test.cpp
    integration/display_controller_sdp_test_single_display.cpp
    integration/transport_test.cpp
)

# Link Google Test and project libraries
if(TARGET gtest_main AND TARGET gtest)
    # Using local GoogleTest
    target_link_libraries(integration_tests
        PRIVATE
        gtest_main
        gtest
        screensdk
    )
elseif(TARGET GTest::gtest_main AND TARGET GTest::gtest)
    # Using system GoogleTest
    target_link_libraries(integration_tests
        PRIVATE
        GTest::gtest_main
        GTest::gtest
        screensdk
    )
endif()

# Add compile definition for static library
target_compile_definitions(integration_tests PRIVATE
    SCREEN_STREAM_SDK_STATIC
)

target_include_directories(integration_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../ScreenStreamSDK/include
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/third_party/x264/include
)

# gtest_discover_tests(integration_tests)  # Temporarily disabled for linking

# End-to-end tests
add_executable(e2e_tests
    e2e/e2e_test_helper.cpp
    e2e/basic_connection_test.cpp
    e2e/video_stream_test.cpp
    e2e/latency_test.cpp
    e2e/stability_test.cpp
)

# Link Google Test and project libraries
if(TARGET gtest_main AND TARGET gtest)
    # Using local GoogleTest
    target_link_libraries(e2e_tests
        PRIVATE
        gtest_main
        gtest
        screensdk
    )
elseif(TARGET GTest::gtest_main AND TARGET GTest::gtest)
    # Using system GoogleTest
    target_link_libraries(e2e_tests
        PRIVATE
        GTest::gtest_main
        GTest::gtest
        screensdk
    )
endif()

# Add compile definition for static library
target_compile_definitions(e2e_tests PRIVATE
    SCREEN_STREAM_SDK_STATIC
)

target_include_directories(e2e_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../ScreenStreamSDK/include
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/third_party/x264/include
)

# gtest_discover_tests(e2e_tests)  # Temporarily disabled for linking

# Debug test program
add_executable(test_dc
    ../ScreenStreamSDK/tests/test_dc.cpp
)

target_link_libraries(test_dc
    PRIVATE
    screensdk
)

# Link libdatachannel
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib")
        target_link_libraries(test_dc
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET test_dc POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/bin/*.dll
                $<TARGET_FILE_DIR:test_dc>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib")
        target_link_libraries(test_dc
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET test_dc POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/bin/*.dll
                $<TARGET_FILE_DIR:test_dc>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
endif()

# Add Windows socket libraries
target_link_libraries(test_dc
    PRIVATE
    ws2_32
    crypt32
    secur32
)

target_include_directories(test_dc
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../ScreenStreamSDK/include
    ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/include
)

# Simple test program
add_executable(test_dc_simple
    ../ScreenStreamSDK/tests/test_dc_simple.cpp
)

target_link_libraries(test_dc_simple
    PRIVATE
    screensdk
)

# Link libdatachannel
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib")
        target_link_libraries(test_dc_simple
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET test_dc_simple POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/bin/*.dll
                $<TARGET_FILE_DIR:test_dc_simple>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib")
        target_link_libraries(test_dc_simple
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET test_dc_simple POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/bin/*.dll
                $<TARGET_FILE_DIR:test_dc_simple>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
endif()

# Add Windows socket libraries
target_link_libraries(test_dc_simple
    PRIVATE
    ws2_32
    crypt32
    secur32
)

target_include_directories(test_dc_simple
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../ScreenStreamSDK/include
    ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/include
)

# Test without callbacks
add_executable(test_dc_no_callback
    ../ScreenStreamSDK/tests/test_dc_no_callback.cpp
)

target_link_libraries(test_dc_no_callback
    PRIVATE
    screensdk
)

# Link libdatachannel
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib")
        target_link_libraries(test_dc_no_callback
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET test_dc_no_callback POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/bin/*.dll
                $<TARGET_FILE_DIR:test_dc_no_callback>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib")
        target_link_libraries(test_dc_no_callback
            PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib
        )
        # Copy DLLs to output directory
        add_custom_command(TARGET test_dc_no_callback POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/bin/*.dll
                $<TARGET_FILE_DIR:test_dc_no_callback>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
endif()

# Add Windows socket libraries
target_link_libraries(test_dc_no_callback
    PRIVATE
    ws2_32
    crypt32
    secur32
)

target_include_directories(test_dc_no_callback
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../ScreenStreamSDK/include
    ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/include
)

# Direct libdatachannel test
add_executable(test_dc_lib
    ../ScreenStreamSDK/tests/test_dc_lib.cpp
)

# Link libdatachannel (try both debug and release)
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib")
    target_link_libraries(test_dc_lib
        PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/lib/datachannel.lib
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/bin/datachannel.dll")
        # Copy DLLs to output directory
        add_custom_command(TARGET test_dc_lib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/debug/bin/*.dll
                $<TARGET_FILE_DIR:test_dc_lib>
            COMMENT "Copying libdatachannel DLLs to output directory")
    endif()
elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib")
    target_link_libraries(test_dc_lib
        PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/lib/datachannel.lib
    )
    # Copy DLLs to output directory
    add_custom_command(TARGET test_dc_lib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/bin/*.dll
            $<TARGET_FILE_DIR:test_dc_lib>
        COMMENT "Copying libdatachannel DLLs to output directory")
endif()

# Add Windows socket libraries
target_link_libraries(test_dc_lib
    PRIVATE
    ws2_32
    crypt32
    secur32
)

target_include_directories(test_dc_lib
    PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows/include
)
