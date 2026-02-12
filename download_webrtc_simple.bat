@echo off
REM WebRTC Simple Download Script
REM Use GitHub mirror source

setlocal enabledelayedexpansion

echo ========================================
echo WebRTC Simple Download Script
echo ========================================
echo.

REM Set download path
set "WEBRTC_DIR=%~dp0third_party\webrtc"
set "GITHUB_WERTC=https://github.com/webrtc-sdk/libwebrtc"

echo [Info] WebRTC will be downloaded to: %WEBRTC_DIR%
echo [Info] Using GitHub mirror source
echo.

REM Check if directory exists
if exist "%WEBRTC_DIR%" (
    echo [Warning] Directory already exists: %WEBRTC_DIR%
    echo.
    set /p confirm="Delete and redownload? (Y/N): "
    if /i not "%confirm%"=="Y" (
        echo [Cancel] Download cancelled
        goto :END
    )
    rmdir /s /q "%WEBRTC_DIR%"
    echo [Info] Old directory deleted
)

REM Create directory
mkdir "%WEBRTC_DIR%"
cd /d "%WEBRTC_DIR%"

echo.
echo [Step 1] Cloning WebRTC repository...
echo Note: This is a simplified version, not complete source code
echo.

REM Clone repository
git clone --depth 1 --branch master %GITHUB_WERTC% src

if errorlevel 1 (
    echo [Error] Clone failed!
    echo.
    echo Possible reasons:
    echo 1. Network connection problems
    echo 2. Git not installed or misconfigured
    echo.
    echo Please check:
    echo 1. Git is installed (run: git --version)
    echo 2. Network connection is normal
    echo.
    goto :END
)

echo [Success] WebRTC library downloaded!
echo.
echo ========================================
echo Next Steps
echo ========================================
echo.
echo WebRTC library downloaded to: %WEBRTC_DIR%\src
echo.
echo Please update CMakeLists.txt to link WebRTC library:
echo   target_link_libraries(your_target webrtc)
echo.
echo Or adjust the path according to actual library file location.
echo.

:END
pause
