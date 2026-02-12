@echo off
REM WebRTC Download Script
REM Supports official source and mirror sources

setlocal enabledelayedexpansion

echo ========================================
echo WebRTC Download Script
echo ========================================
echo.

REM Set download path
set "WEBRTC_DIR=%~dp0third_party\webrtc"
set "DEPOT_TOOLS_DIR=%~dp0third_party\depot_tools"

REM Check if using mirror source
set "USE_MIRROR=0"
if "%1"=="--mirror" set "USE_MIRROR=1"
if "%1"=="-m" set "USE_MIRROR=1"

REM Check parameters
if "%USE_MIRROR%"=="1" (
    echo [Config] Using mirror source
    set "WEBRTC_REPO=https://chromium.googlesource.com"
    REM Can be replaced with domestic mirror
    REM set "WEBRTC_REPO=https://source.codeaurora.org/external/imx"
) else (
    echo [Config] Using official source
    set "WEBRTC_REPO=https://chromium.googlesource.com"
)

echo.
echo [Info] WebRTC will be downloaded to: %WEBRTC_DIR%
echo [Info] Depot Tools will be downloaded to: %DEPOT_TOOLS_DIR%
echo.

REM Method 1: Use Depot Tools (Recommended)
echo ========================================
echo Method 1: Use Depot Tools to download (Recommended)
echo ========================================
echo.

REM Check if depot_tools exists
if not exist "%DEPOT_TOOLS_DIR%" (
    echo [Step 1] Downloading Depot Tools...
    
    REM Create directory
    if not exist "%DEPOT_TOOLS_DIR%" (
        mkdir "%DEPOT_TOOLS_DIR%"
    )
    
    REM Try git clone
    echo Cloning depot_tools from GitHub...
    git clone https://github.com/chromium/depot_tools.git "%DEPOT_TOOLS_DIR%"
    
    if errorlevel 1 (
        echo [Error] Failed to download depot_tools!
        echo.
        goto :METHOD2
    )
    
    echo [Success] Depot Tools downloaded
) else (
    echo [Skip] Depot Tools already exists
)

REM Add to PATH
set "PATH=%DEPOT_TOOLS_DIR%;%PATH%"

echo.
echo [Step 2] Initializing WebRTC working directory...
cd /d "%~dp0"

REM Create .gclient config file
echo solutions = [ > .gclient
echo   { >> .gclient
echo     "name": "src", >> .gclient
echo     "url": "%WEBRTC_REPO%/external/webrtc.git", >> .gclient
echo     "deps_file": "DEPS", >> .gclient
echo     "managed": False, >> .gclient
echo     "custom_deps": {}, >> .gclient
echo   }, >> .gclient
echo ] >> .gclient

echo [Success] .gclient config file created
echo.

echo [Step 3] Syncing WebRTC code...
echo Note: This will take a long time (possibly hours) and require a lot of disk space (~10GB+)
echo.
set /p confirm="Continue? (Y/N): "
if /i not "%confirm%"=="Y" (
    echo [Cancel] Download cancelled
    goto :END
)

REM Sync code
gclient sync --no-history

if errorlevel 1 (
    echo.
    echo [Error] Failed to sync WebRTC code!
    echo.
    echo Possible reasons:
    echo 1. Network connection problems
    echo 2. chromium.googlesource.com access restricted
    echo 3. Insufficient disk space
    echo.
    echo Solutions:
    echo 1. Check network connection
    echo 2. Use proxy or VPN
    echo 3. Use domestic mirror (run: download_webrtc.bat --mirror)
    echo 4. Consider using pre-built WebRTC library
    goto :METHOD2
) else (
    echo.
    echo [Success] WebRTC code downloaded!
    goto :END
)

:METHOD2
echo.
echo ========================================
echo Method 2: Use pre-built library (Alternative)
echo ========================================
echo.

echo Since downloading WebRTC source code takes a long time and network connection, 
echo it is recommended to use pre-built libraries:
echo.
echo Option A: Use libwebrtc (C++ wrapper library)
echo   - GitHub: https://github.com/mm2/Low-Latency-HTTP-Live-Streaming/tree/master/webrtc-native
echo   - More lightweight, suitable for embedded applications
echo.
echo Option B: Use libdatachannel (WebRTC data channel library)
echo   - GitHub: https://github.com/paullouisageneau/libdatachannel
echo   - Simpler, focused on data channels
echo   - CMake build, easier to integrate
echo.
echo Option C: Use Pion WebRTC (Go implementation)
echo   - GitHub: https://github.com/pion/webrtc
echo   - If Go interface is needed
echo.

goto :END

:END
echo.
echo ========================================
echo WebRTC Download Script Ended
echo ========================================
echo.
pause
