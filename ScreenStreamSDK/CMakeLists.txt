cmake_minimum_required(VERSION 3.15)
project(ScreenStreamSDK VERSION 1.0.0 LANGUAGES CXX)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")

# Do not treat warnings as errors
string(REPLACE "/WX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/WX" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REPLACE "/WX" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann)
include_directories(${CMAKE_SOURCE_DIR}/third_party/cpp-httplib)
include_directories(${CMAKE_SOURCE_DIR}/../third_party/x264/include)

# x264 library integration
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/x264/include/x264.h")
    message(STATUS "Using local x264: ${CMAKE_SOURCE_DIR}/../third_party/x264")
    include_directories(${CMAKE_SOURCE_DIR}/../third_party/x264/include)
    link_directories(${CMAKE_SOURCE_DIR}/../third_party/x264/lib/x64)
    set(X264_LIBRARIES x264)
else()
    message(WARNING "x264 not found in third_party/x264")
endif()

# libdatachannel library integration
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows")
    message(STATUS "Using libdatachannel: ${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows")
    include_directories(${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows/include)
    link_directories(${CMAKE_SOURCE_DIR}/../third_party/webrtc/libdatachannel_x64-windows/lib)
    set(LIBDATACHANNEL_LIBRARIES datachannel)
else()
    message(WARNING "libdatachannel not found in third_party/webrtc/libdatachannel_x64-windows")
endif()

# NVENC SDK integration
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/nvenc/Interface/nvEncodeAPI.h")
    message(STATUS "NVENC SDK found, hardware encoding enabled")
    add_definitions(-DHAS_NVENC)
    include_directories(${CMAKE_SOURCE_DIR}/../third_party/nvenc/Interface)
    link_directories(${CMAKE_SOURCE_DIR}/../third_party/nvenc/x64)
else()
    message(WARNING "NVENC SDK not found, hardware encoding disabled")
endif()

# Windows libraries
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)  # Windows 10+ (required by cpp-httplib v0.30.1)
    link_libraries(d3d11 dxgi user32 gdi32)
endif()

# Subdirectories
add_subdirectory(src)

# Simple test programs
add_executable(test_dxgi_display tests/test_dxgi_display.cpp)
target_link_libraries(test_dxgi_display PRIVATE screensdk)

# cpp-httplib integration test
add_executable(test_httplib ../tests/test_httplib.cpp)
target_include_directories(test_httplib PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib
)
target_link_libraries(test_httplib PRIVATE screensdk)

if(EXISTS "${CMAKE_SOURCE_DIR}/../tests/CMakeLists.txt")
    # Explicitly specify binary directory for out-of-tree source
    add_subdirectory(${CMAKE_SOURCE_DIR}/../tests ${CMAKE_BINARY_DIR}/tests)
endif()

# Export symbols for DLL
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
