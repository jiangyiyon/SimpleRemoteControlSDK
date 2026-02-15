@echo off
REM ============================================
REM RemoteDesktopServer Test Launcher
REM For T1004 - Real Device Testing
REM ============================================

setlocal enabledelayedexpansion

echo ============================================
echo   RemoteDesktopServer Test Launcher
echo ============================================
echo.

REM Configuration
set CONFIG_FILE=config\test_server_config.json
set LOG_FILE=test_server_log.txt

REM Check if config file exists
if not exist "%CONFIG_FILE%" (
    echo Creating default configuration...
    echo {> "%CONFIG_FILE%"
    echo   "http_port": 8080,>> "%CONFIG_FILE%"
    echo   "signaling_port": 8081,>> "%CONFIG_FILE%"
    echo   "web_root": "web",>> "%CONFIG_FILE%"
    echo   "display_id": 0,>> "%CONFIG_FILE%"
    echo   "fps": 30,>> "%CONFIG_FILE%"
    echo   "max_bitrate_bps": 15000000,>> "%CONFIG_FILE%"
    echo   "stun_server": "stun:stun.l.google.com:19302">> "%CONFIG_FILE%"
    echo }>> "%CONFIG_FILE%"
    echo Configuration file created: %CONFIG_FILE%
    echo.
)

REM Get local IP address
echo Getting local IP address...
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| findstr /C:"IPv4 Address"') do (
    set LOCAL_IP=%%a
    set LOCAL_IP=!LOCAL_IP: =!
    goto :ip_found
)
:ip_found

echo Local IP Address: %LOCAL_IP%
echo.

REM Display network instructions
echo ============================================
echo   Network Setup Instructions
echo ============================================
echo.
echo 1. Ensure your PC and mobile device are on the same network
echo.
echo 2. Configure Windows Firewall:
echo    - Open Windows Defender Firewall
echo    - Allow app: RemoteDesktopServer (or build\bin\Debug\integration_tests.exe)
echo    - Allow inbound connections on ports 8080 and 8081
echo.
echo 3. On your mobile device, open browser and navigate to:
echo    http://%LOCAL_IP%:8080
echo.
echo 4. For desktop testing, navigate to:
echo    http://localhost:8080
echo.
echo ============================================
echo.

REM Check if server executable exists
set SERVER_EXE=build\bin\Debug\integration_tests.exe
if not exist "%SERVER_EXE%" (
    echo ERROR: Server executable not found!
    echo Expected: %SERVER_EXE%
    echo Please build the project first using:
    echo   cmake --build build --config Debug
    pause
    exit /b 1
)

echo Starting RemoteDesktopServer...
echo Configuration: %CONFIG_FILE%
echo Log file: %LOG_FILE%
echo.
echo Press Ctrl+C to stop the server
echo ============================================
echo.

REM Start server (using integration_tests which includes RemoteDesktopServer test)
REM Note: In production, we would have a dedicated RemoteDesktopServer executable
echo To test manually:
echo 1. Run: build\bin\Debug\integration_tests.exe
echo 2. The tests will start and stop the server automatically
echo.
echo For manual server testing, you need to:
echo 1. Create a standalone server executable
echo 2. Or use the test with RemoteDesktopIntegrationTest.FullStackTest
echo.

pause

REM For now, let's show how to run the integration test
echo.
echo Running integration test with FullStackTest...
echo This will start the server for ~7 seconds
echo.
"%SERVER_EXE%" --gtest_filter=RemoteDesktopIntegrationTest.FullStackTest

echo.
echo ============================================
echo   Test Complete
echo ============================================
echo.
echo Server has been stopped.
echo Check test results above.
echo.
echo For real device testing:
echo 1. You need a dedicated server executable
echo 2. Or modify the test to keep server running longer
echo 3. See t1004_test_plan.md for test procedures
echo.

pause
