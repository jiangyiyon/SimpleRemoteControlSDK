@echo off
REM Generate Visual Studio 2022 solution for RemoteControlSDK

echo ========================================
echo Generating Visual Studio 2022 Solution
echo ========================================
echo.

REM Check if build directory exists, if not create it
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Generate VS2026 solution
echo Generating solution with CMake...
cmake -B build -S ScreenStreamSDK -G "Visual Studio 18 2026" -A x64

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Solution generated successfully!
    echo Solution file: build\RemoteControlSDK.sln
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Failed to generate solution!
    echo ========================================
    exit /b %ERRORLEVEL%
)

pause
