@echo off

rem Script to directly clone and build WebRTC from official repository
rem Installs WebRTC to E:\TestWebRTC\RemoteControlSDK\third_party\webrtc directory

setlocal

rem Set project root directory
set PROJECT_ROOT=E:\TestWebRTC\RemoteControlSDK

rem Set WebRTC installation directory
set WEBRTC_INSTALL_DIR=%PROJECT_ROOT%\third_party\webrtc

rem Set build output directory
set WEBRTC_BUILD_DIR=%WEBRTC_INSTALL_DIR%\out\Release

rem Create WebRTC installation directory
if not exist "%WEBRTC_INSTALL_DIR%" (
    echo Creating WebRTC installation directory...
    mkdir "%WEBRTC_INSTALL_DIR%"
)

echo WebRTC Setup Script
echo ===================
echo This script will:
echo 1. Clone the WebRTC repository from Chromium's official source
echo 2. Run build hooks to prepare the build environment
echo 3. Provide instructions for building WebRTC
echo ===================
echo 

rem Check if Git is installed
echo Checking for Git...
where git >nul 2>nul
if errorlevel 1 (
    echo Error: Git is not installed. Please install Git before running this script.
    echo Download Git from: https://git-scm.com/downloads
    pause
    exit /b 1
)
echo Git found.

rem Check if Python is installed
echo Checking for Python...
where python >nul 2>nul
if errorlevel 1 (
    echo Error: Python is not installed. Please install Python 3.7+ before running this script.
    echo Download Python from: https://www.python.org/downloads/
    pause
    exit /b 1
)
echo Python found.

rem Clone WebRTC repository
echo 
echo Cloning WebRTC repository from Chromium's official source...
echo This may take a long time (several hours) depending on your network speed.
echo The repository is very large (multiple gigabytes).
echo 
pause

cd "%WEBRTC_INSTALL_DIR%"

rem Check if directory exists
if exist ".git" (
    echo WebRTC repository already exists. Updating...
    git pull
    if errorlevel 1 (
        echo Failed to update WebRTC repository!
        pause
        exit /b 1
    )
) else if exist "." (
    rem Directory exists but isn't a git repository
    echo Directory exists but isn't a git repository. Cleaning up...
    rem Remove all files except .git (which doesn't exist)
    for /d %%i in (*) do rd /s /q "%%i"
    for %%i in (*) do del /f /q "%%i"
    
    echo Cloning WebRTC repository...
    git clone https://chromium.googlesource.com/external/webrtc .
    if errorlevel 1 (
        echo Failed to clone WebRTC repository!
        echo Possible issues:
        echo 1. Network connectivity problems
        echo 2. Git not properly configured
        echo 3. Insufficient disk space
        pause
        exit /b 1
    )
) else (
    echo Cloning WebRTC repository...
    git clone https://chromium.googlesource.com/external/webrtc .
    if errorlevel 1 (
        echo Failed to clone WebRTC repository!
        echo Possible issues:
        echo 1. Network connectivity problems
        echo 2. Git not properly configured
        echo 3. Insufficient disk space
        pause
        exit /b 1
    )
)

echo 
echo Repository cloned successfully!

rem Run WebRTC build hooks
echo Running WebRTC build hooks...
echo This will download additional dependencies.
echo 

call python build/runhooks.py
if errorlevel 1 (
    echo Failed to run build hooks!
    echo Possible issues:
    echo 1. Network connectivity problems
    echo 2. Python dependencies missing
    echo 3. Insufficient disk space
    pause
    exit /b 1
)

echo Build hooks completed successfully!

rem Create build directory if it doesn't exist
if not exist "%WEBRTC_BUILD_DIR%" (
    mkdir "%WEBRTC_BUILD_DIR%"
)

echo 
echo ===================
echo WebRTC Setup Complete!
echo ===================
echo 
echo Next steps to build WebRTC as a dynamic library:
echo 

echo 1. Open a command prompt and navigate to:
echo    %WEBRTC_INSTALL_DIR%
echo 
echo 2. Run the following commands to generate build files:
echo    gn gen out/Release --args="is_debug=false is_component_build=true rtc_include_tests=false"
echo 
echo 3. Build WebRTC using Ninja:
echo    ninja -C out/Release

echo 
echo After successful build, the dynamic library files will be in:
echo    %WEBRTC_BUILD_DIR%
echo 
echo Key files you'll need:
echo - webrtc.lib (import library)
echo - webrtc.dll (dynamic library)
echo - Include files in the "include" directory
echo 

echo For more information, visit:
echo https://webrtc.github.io/webrtc-org/native-code/development/

echo 
pause
exit /b 0