cmake_minimum_required(VERSION 3.15)
project(ScreenStreamSDK VERSION 1.0.0 LANGUAGES CXX)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/x264/include)

# x264 library integration
find_package(x264 REQUIRED)
if(x264_FOUND)
    include_directories(${x264_INCLUDE_DIRS})
    link_directories(${x264_LIBRARY_DIRS})
    message(STATUS "Found x264: ${x264_LIBRARIES}")
else()
    message(WARNING "x264 not found, will search manually")
    # Manual x264 path (fallback)
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/x264/lib")
        include_directories(${CMAKE_SOURCE_DIR}/third_party/x264/include)
        link_directories(${CMAKE_SOURCE_DIR}/third_party/x264/lib)
    endif()
endif()

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Export symbols for DLL
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
