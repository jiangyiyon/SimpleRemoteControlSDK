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
    # Unit tests for utils
    unit/error_test.cpp
    unit/metrics_collector_test.cpp
    unit/thread_pool_test.cpp

    # Unit tests for capture
    unit/display_detector_test.cpp
    unit/capture/dxgi_capture_test.cpp

    # Unit tests for encoding
    unit/encoder_config_test.cpp
    unit/encoder_factory_test.cpp
    unit/encoding/x264_encoder_test.cpp

    # Unit tests for core components
    unit/core/session_test.cpp
    unit/core/session_manager_test.cpp

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
gtest_discover_tests(unit_tests)

# Integration tests
add_executable(integration_tests
    integration/capture_encoding_test.cpp
    integration/capture_encoder_test.cpp
    integration/encoding_fallback_test.cpp
    integration/transport_test.cpp
    integration/display_switch_test.cpp
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

gtest_discover_tests(integration_tests)

# End-to-end tests
add_executable(e2e_tests
    e2e/basic_connection_test.cpp
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

gtest_discover_tests(e2e_tests)
