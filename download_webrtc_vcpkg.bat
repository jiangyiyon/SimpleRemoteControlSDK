@echo off
REM Use vcpkg to download WebRTC related libraries
REM This is a recommended solution as vcpkg is easier to integrate

setlocal enabledelayedexpansion

echo ========================================
echo Use vcpkg to download WebRTC related libraries
echo ========================================
echo.

REM Set paths
set "VCPKG_DIR=%~dp0vcpkg"
set "VCPKG_SCRIPT=%VCPKG_DIR%\vcpkg.exe"

echo [Info] vcpkg directory: %VCPKG_DIR%
echo.

REM Check if vcpkg is installed
if not exist "%VCPKG_SCRIPT%" (
    echo [Error] vcpkg not found!
    echo.
    echo Please install vcpkg first:
    echo   git clone https://github.com/Microsoft/vcpkg.git vcpkg
    echo   cd vcpkg
    echo   .\bootstrap-vcpkg.bat
    echo.
    goto :END
)

REM Use vcpkg to install WebRTC related libraries
echo.
echo ========================================
echo Available WebRTC related libraries
echo ========================================
echo.
echo 1. libdatachannel - WebRTC data channel library (Recommended)
echo    Lightweight, easy to integrate
echo.
echo 2. webrtc - Complete WebRTC library
echo    Full featured, but large in size
echo.
echo 3. jsoncpp - JSON parsing library (WebRTC needs)
echo    WebRTC protocol usually needs JSON
echo.
echo 4. boost-asio - Async network library
echo    WebRTC signaling server needs
echo.
echo.

set /p choice="Please select libraries to install (1-4, separate with comma, or enter all to install all): "

echo.
echo [Start] Installing libraries using vcpkg...
echo.

REM Process selection
if "%choice%"=="1" (
    echo [Installing] libdatachannel...
    "%VCPKG_SCRIPT%" install libdatachannel:x64-windows
) else if "%choice%"=="2" (
    echo [Installing] webrtc...
    "%VCPKG_SCRIPT%" install webrtc:x64-windows
) else if "%choice%"=="3" (
    echo [Installing] jsoncpp...
    "%VCPKG_SCRIPT%" install jsoncpp:x64-windows
) else if "%choice%"=="4" (
    echo [Installing] boost-asio...
    "%VCPKG_SCRIPT%" install boost-asio:x64-windows
) else if "%choice%"=="all" (
    echo [Installing] All libraries...
    "%VCPKG_SCRIPT%" install libdatachannel:x64-windows jsoncpp:x64-windows boost-asio:x64-windows
) else (
    echo [Error] Invalid selection!
    goto :END
)

if errorlevel 1 (
    echo.
    echo [Error] Installation failed!
    echo.
    echo Possible reasons:
    echo 1. vcpkg not properly configured
    echo 2. Network connection problems
    echo 3. Dependency library does not exist
    echo.
    echo Solutions:
    echo 1. Update vcpkg: cd vcpkg ^&^& git pull ^&^& .\bootstrap-vcpkg.bat
    echo 2. Search available libraries: vcpkg search webrtc
    echo 3. View specific library: vcpkg show libdatachannel
    goto :END
)

echo.
echo [Success] Library installation complete!
echo.
echo ========================================
echo Configure CMake
echo ========================================
echo.
echo To use vcpkg libraries in CMake, you need:
echo.
echo 1. Toolchain file: vcpkg/scripts/buildsystems/vcpkg.cmake
echo 2. CMake configuration: cmake -DCMAKE_TOOLCHAIN_FILE=...
echo.
echo Example:
echo   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
echo     -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
echo.
echo 3. In CMakeLists.txt:
echo    find_package(libdatachannel CONFIG REQUIRED)
echo    target_link_libraries(your_target libdatachannel::libdatachannel)
echo.

:END
echo.
pause
