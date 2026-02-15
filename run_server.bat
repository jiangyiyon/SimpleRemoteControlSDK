@echo off
REM ============================================
REM RemoteDesktopServer Launcher
REM ============================================

echo.
echo ============================================
echo   RemoteDesktopServer - Standalone Mode
echo ============================================
echo.

REM Set ports
set HTTP_PORT=8080
set SIGNALING_PORT=8081

echo Starting server on:
echo   HTTP:      http://localhost:%HTTP_PORT%
echo   Signaling: http://localhost:%SIGNALING_PORT%
echo.
echo Press Ctrl+C to stop the server
echo ============================================
echo.

REM Run integration test with modified behavior
REM The FullStackTest keeps server running for ~7 seconds
build\bin\Debug\integration_tests.exe --gtest_filter=RemoteDesktopIntegrationTest.FullStackTest

echo.
echo ============================================
echo   Server Stopped
echo ============================================
echo.

pause
