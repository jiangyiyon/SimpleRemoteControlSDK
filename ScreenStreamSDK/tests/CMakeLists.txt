# First try to use local GoogleTest from third_party
# Calculate the correct path to third_party/googletest
# Note: When included from ScreenStreamSDK/CMakeLists.txt, CMAKE_SOURCE_DIR is ScreenStreamSDK
if(EXISTS "${CMAKE_SOURCE_DIR}/../third_party/googletest/CMakeLists.txt")
    # Case 1: Included from ScreenStreamSDK/CMakeLists.txt
    set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/../third_party/googletest")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party/googletest/CMakeLists.txt")
    # Case 2: Directly running from tests directory
    set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/third_party/googletest")
else
    # Default path
    set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/third_party/googletest")
endif()
set(GTEST_FOUND FALSE)

# Check if local GoogleTest exists and has CMakeLists.txt
if(EXISTS "${GTEST_ROOT}/CMakeLists.txt")
    message(STATUS "Using local GoogleTest: ${GTEST_ROOT}")
    add_subdirectory(${GTEST_ROOT} ${CMAKE_BINARY_DIR}/googletest)
    set(GTEST_FOUND TRUE)
elseif(EXISTS "${GTEST_ROOT}/googletest/CMakeLists.txt")
    # Common case: googletest is nested inside a googletest directory
    message(STATUS "Using nested local GoogleTest: ${GTEST_ROOT}/googletest")
    add_subdirectory(${GTEST_ROOT}/googletest ${CMAKE_BINARY_DIR}/googletest)
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
        message(WARNING "You can download it from: https://github.com/google/googletest/releases")
        message(WARNING "Extract the contents into third_party/googletest directory.")
        return()
    endif()
endif()

if(GTEST_FOUND)
    message(STATUS "Building unit tests with GoogleTest")

    # Set test output directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    # Unit tests
    add_subdirectory(unit)
else()
    message(WARNING "GoogleTest not found, skipping tests")
    return()
endif()
