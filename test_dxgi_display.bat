@echo off
REM Build and run DisplayDetector DXGI test

echo ========================================
echo Building DisplayDetector DXGI Test
echo ========================================

REM Check if build directory exists
if not exist "ScreenStreamSDK\build" (
    echo Creating build directory...
    mkdir ScreenStreamSDK\build
)

REM Configure CMake
cd ScreenStreamSDK
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

REM Build test_dxgi_display
cmake --build build --config Debug --target test_dxgi_display
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Running DisplayDetector DXGI Test
echo ========================================
echo.

cd ..\build\bin\Debug
test_dxgi_display.exe
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Test failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo All tests passed!
echo ========================================

pause
