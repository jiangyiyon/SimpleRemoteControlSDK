@echo off
REM Build script for T047 - screensdk::createSession() API

echo.
echo ========================================
echo Building T047: screensdk API
echo ========================================
echo.

cd build

REM Clean previous build
if exist CMakeCache.txt (
  echo Cleaning previous build...
  del /Q CMakeCache.txt
)

REM Configure CMake
echo Configuring CMake...
cmake -G "Visual Studio 18 2026" -A x64 ..\ScreenStreamSDK
if errorlevel 1 (
  echo ERROR: CMake configuration failed
  cd ..
  exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Debug
if errorlevel 1 (
  echo ERROR: Build failed
  cd ..
  exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.

cd ..

REM Run unit tests
echo.
echo ========================================
echo Running unit tests...
echo ========================================
echo.
cd build
.\Debug\unit_tests.exe --gtest_filter=ScreensdkApiTest.*
if errorlevel 1 (
  echo WARNING: Some tests failed
  cd ..
  exit /b 1
)

cd ..
echo.
echo ========================================
echo All tests passed!
echo ========================================
echo.

exit /b 0
