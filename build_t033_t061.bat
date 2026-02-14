@echo off
echo ========================================
echo Build T033 (Session) and T061 (Session Display Selection)
echo ========================================
echo.

cd /d %~dp0\ScreenStreamSDK

echo [1/3] Configuring CMake...
if not exist build (
  mkdir build
)
cd build
cmake -G "Visual Studio 18 2026" -A x64 ..
if %errorlevel% neq 0 (
  echo ERROR: CMake configuration failed
  pause
  exit /b 1
)

echo.
echo [2/3] Building project...
cmake --build . --config Debug --target screensdk
if %errorlevel% neq 0 (
  echo ERROR: Build failed
  pause
  exit /b 1
)

echo.
echo [3/3] Running unit tests...
cd /d %~dp0\tests\build
if not exist build (
  echo WARNING: Tests build directory not found
  echo Running from ScreenStreamSDK/build instead...
  cd /d %~dp0\ScreenStreamSDK\build
)

ctest --output-on-failure -R "session_test|session_display_test"
if %errorlevel% neq 0 (
  echo ERROR: Tests failed
  pause
  exit /b 1
)

echo.
echo ========================================
echo SUCCESS: T033 and T061 completed
echo ========================================
pause
