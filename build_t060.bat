@echo off
REM Simple build script for T060 - DisplayDetector DXGI implementation

echo ========================================
echo Building DisplayDetector DXGI Implementation
echo ========================================
echo.

cd ScreenStreamSDK

REM Create build directory if not exists
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Configure CMake
echo Configuring CMake...
cmake -B build -S . -G "Visual Studio 18 2026" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    echo Please install Visual Studio 2022 or update the generator.
    pause
    exit /b 1
)

REM Build test_dxgi_display target
echo.
echo Building test_dxgi_display...
cmake --build build --config Debug --target test_dxgi_display
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Running DisplayDetector DXGI Tests
echo ========================================
echo.

cd build\bin\Debug
test_dxgi_display.exe
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Test failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS: All tests passed!
echo ========================================
echo.

pause
