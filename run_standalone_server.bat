@echo off
REM ============================================
REM RemoteDesktopServer - Standalone Server
REM ============================================

echo.
echo ============================================
echo   Remote Desktop Server - Standalone
echo ============================================
echo.

REM Set default ports
set HTTP_PORT=18080
set SIGNALING_PORT=18081

echo Starting server on:
echo   HTTP:      http://localhost:%HTTP_PORT%
echo   Signaling: ws://localhost:%SIGNALING_PORT%
echo   Web:       http://localhost:%HTTP_PORT%/index.html
echo.
echo Press Ctrl+C to stop the server
echo ============================================
echo.

REM Run standalone server
build\bin\Debug\standalone_server.exe ^
    --http-port %HTTP_PORT% ^
    --signaling-port %SIGNALING_PORT% ^
    --web-root web ^
    --display-id 1 ^
    --fps 30 ^
    --max-bitrate 15000000

echo.
echo ============================================
echo   Server Stopped
echo ============================================
echo.

pause
