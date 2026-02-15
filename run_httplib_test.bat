@echo off
echo ========================================
echo cpp-httplib Integration Test
echo ========================================
echo.
echo Starting test server on port 18080...
echo Press Ctrl+C to stop
echo.
echo After starting, you can test:
echo   - http://localhost:18080/
echo   - http://localhost:18080/status
echo   - http://localhost:18080/test-cors
echo.
e:\TestWebRTC\RemoteControlSDK\bin\Debug\test_httplib.exe
