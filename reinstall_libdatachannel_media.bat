@echo off
REM Reinstall libdatachannel with media support using vcpkg

setlocal enabledelayedexpansion

echo ========================================
echo Reinstalling libdatachannel with media support
echo ========================================
echo.

REM Set paths
set "VCPKG_ROOT=E:\vcg\vcpkg"
set "VCPKG_SCRIPT=%VCPKG_ROOT%\vcpkg.exe"

echo [Info] vcpkg directory: %VCPKG_ROOT%
echo.

REM Check if vcpkg exists
if not exist "%VCPKG_SCRIPT%" (
    echo [Error] vcpkg not found at: %VCPKG_SCRIPT%
    goto :END
)

echo.
echo ========================================
echo Removing old libdatachannel
echo ========================================
echo.

cd "%VCPKG_ROOT%"
"%VCPKG_SCRIPT%" remove libdatachannel:x64-windows --recurse

if errorlevel 1 (
    echo [Warning] Remove failed or package not installed, continuing...
)

echo.
echo ========================================
echo Installing libdatachannel with srtp (media) support
echo ========================================
echo.

REM Install with srtp feature enabled (enables media support)
"%VCPKG_SCRIPT%" install libdatachannel[ws,srtp]:x64-windows

if errorlevel 1 (
    echo [Error] Installation failed!
    goto :END
)

echo.
echo [Success] libdatachannel with media support installed successfully!
echo.
echo ========================================
echo Next Steps
echo ========================================
echo.
echo 1. Rebuild your project:
echo    cd build
echo    cmake --build . --config Release
echo.
echo 2. Run the server again
echo.
echo Note: The new libdatachannel will be used automatically
echo from the vcpkg installed directory.
echo.

:END
echo.
pause
