@echo off
REM Build libdatachannel with media support using vcpkg

setlocal enabledelayedexpansion

echo ========================================
echo Building libdatachannel with media support
echo ========================================
echo.

REM Set paths
set "VCPKG_ROOT=E:\vcg\vcpkg"
set "VCPKG_SCRIPT=%VCPKG_ROOT%\vcpkg.exe"
set "PROJECT_ROOT=%~dp0"
set "BUILD_DIR=%PROJECT_ROOT%third_party\webrtc\libdatachannel_build"
set "INSTALL_DIR=%PROJECT_ROOT%third_party\webrtc\libdatachannel_x64-windows_media"

echo [Info] vcpkg directory: %VCPKG_ROOT%
echo [Info] Project root: %PROJECT_ROOT%
echo [Info] Build directory: %BUILD_DIR%
echo [Info] Install directory: %INSTALL_DIR%
echo.

REM Check if vcpkg exists
if not exist "%VCPKG_SCRIPT%" (
    echo [Error] vcpkg not found at: %VCPKG_SCRIPT%
    goto :END
)

REM Create build directory
if exist "%BUILD_DIR%" (
    echo [Info] Cleaning existing build directory...
    rmdir /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"

REM Create install directory
if exist "%INSTALL_DIR%" (
    echo [Info] Cleaning existing install directory...
    rmdir /s /q "%INSTALL_DIR%"
)
mkdir "%INSTALL_DIR%"

echo.
echo ========================================
echo Configuring CMake with media support
echo ========================================
echo.

REM Configure with media support enabled
cd "%BUILD_DIR%"
set "SOURCE_DIR=%VCPKG_ROOT%/buildtrees/libdatachannel/src/v0.24.0-1f6f1b6308.clean"

echo [Info] Source directory: %SOURCE_DIR%
echo.

cmake "%SOURCE_DIR%" ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL ^
    -DRTC_MEDIA=ON ^
    -DRTC_WEBSOCKET=ON ^
    -DRTC_DATA_CHANNEL=ON ^
    -DNO_MEDIA=OFF ^
    -DNO_WEBSOCKET=OFF

echo.
echo ========================================
echo Building libdatachannel
echo ========================================
echo.

REM Build
cmake --build . --config Release -j

if errorlevel 1 (
    echo [Error] Build failed!
    goto :END
)

echo.
echo ========================================
echo Installing libdatachannel
echo ========================================
echo.

REM Install
cmake --install . --config Release

if errorlevel 1 (
    echo [Error] Install failed!
    goto :END
)

echo.
echo [Success] libdatachannel with media support built successfully!
echo.
echo Installed to: %INSTALL_DIR%
echo.
echo ========================================
echo Update CMakeLists.txt
echo ========================================
echo.
echo You need to update CMakeLists.txt to use the new libdatachannel:
echo.
echo In ScreenStreamSDK/CMakeLists.txt, change:
echo   set(LIBDATACHANNEL_DIR ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows)
echo to:
echo   set(LIBDATACHANNEL_DIR ${CMAKE_SOURCE_DIR}/third_party/webrtc/libdatachannel_x64-windows_media)
echo.

:END
echo.
pause
