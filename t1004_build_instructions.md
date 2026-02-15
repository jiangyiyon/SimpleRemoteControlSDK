# T1004 Build Instructions

## Building the Test Server

Since this project uses Visual Studio project files (CMake generated), here's how to build a test server:

### Option 1: Using Existing Tests (Quickest)

The integration tests already include RemoteDesktopServer. You can use `FullStackTest` to start the server.

```bash
# Run the full stack test (keeps server running for ~7 seconds)
build\bin\Debug\integration_tests.exe --gtest_filter=RemoteDesktopIntegrationTest.FullStackTest

# Run all remote desktop integration tests
build\bin\Debug\integration_tests.exe --gtest_filter=RemoteDesktopIntegrationTest:*
```

### Option 2: Build test_server_main.exe

1. **Add test_server_main.cpp to CMakeLists.txt**

   Edit `tests/CMakeLists.txt` and add:

   ```cmake
   # Test server executable
   add_executable(test_server test_server_main.cpp)
   target_link_libraries(test_server PRIVATE screensdk gtest)
   ```

2. **Build using CMake**

   ```bash
   cmake --build build --config Debug --target test_server
   ```

3. **Run the test server**

   ```bash
   build\bin\Debug\test_server.exe
   ```

### Option 3: Manual Build with Visual Studio

1. Open `ScreenStreamSDK.sln` in Visual Studio
2. Add `test_server_main.cpp` to the project
3. Right-click on project → Add → Existing Item
4. Select `test_server_main.cpp`
5. Build the project (F7 or Ctrl+Shift+B)
6. Run the executable

## Quick Start

### 1. Start the Server

```bash
# Using integration test (temporary, ~7 seconds)
build\bin\Debug\integration_tests.exe --gtest_filter=RemoteDesktopIntegrationTest.FullStackTest

# OR using test_server.exe (continuous)
build\bin\Debug\test_server.exe
```

### 2. Get PC IP Address

```bash
ipconfig
```

Look for "IPv4 Address" (e.g., 192.168.1.100)

### 3. Configure Firewall

**Windows Defender Firewall:**

1. Open Windows Defender Firewall
2. Click "Allow an app or feature through Windows Defender Firewall"
3. Click "Change settings" (requires admin)
4. Allow:
   - `integration_tests.exe` (if using integration test)
   - `test_server.exe` (if using test server)
   - Check both Private and Public networks
5. Allow inbound connections on ports 8080 and 8081

**Command Line (Admin):**

```bash
# Allow port 8080
netsh advfirewall firewall add rule name="RemoteDesktopServer HTTP" dir=in action=allow protocol=TCP localport=8080

# Allow port 8081
netsh advfirewall firewall add rule name="RemoteDesktopServer Signaling" dir=in action=allow protocol=TCP localport=8081

# Allow the application
netsh advfirewall firewall add rule name="RemoteDesktopServer App" dir=in action=allow program="build\bin\Debug\integration_tests.exe" enable=yes
```

### 4. Access from Devices

**Desktop Browser:**
```
http://localhost:8080
```

**Mobile Device (Android/iOS):**
```
http://<PC_IP>:8080
```

Example:
```
http://192.168.1.100:8080
```

## Testing Checklist

### Desktop Browser Testing
- [ ] Chrome: Navigate to `http://localhost:8080`
- [ ] Click "Connect"
- [ ] Verify video stream appears
- [ ] Test mouse interactions
- [ ] Test keyboard input
- [ ] Measure latency

- [ ] Edge: Repeat steps above
- [ ] Firefox: Repeat steps above

### Mobile Device Testing
- [ ] Ensure mobile device and PC are on same network
- [ ] Get PC IP address
- [ ] On Android (Chrome) or iOS (Safari), navigate to `http://<PC_IP>:8080`
- [ ] Click "Connect"
- [ ] Verify video stream appears
- [ ] Test touch interactions
- [ ] Measure latency
- [ ] Test different orientations (portrait/landscape)

## Troubleshooting

### Server doesn't start
- Check if port 8080/8081 is already in use
- Run `netstat -ano | findstr :8080` to check
- Change ports in configuration if needed

### Cannot access from mobile device
- Verify both devices are on same network (same subnet)
- Check Windows Firewall settings
- Try pinging PC from mobile device
- Disable antivirus temporarily to test

### Video doesn't appear
- Check browser console for errors
- Verify WebRTC is supported (modern browsers)
- Check STUN server is reachable
- Try different browser

### High latency
- Check network connection speed
- Reduce video quality (FPS, bitrate)
- Try wired network connection instead of Wi-Fi

## Performance Tuning

If you experience performance issues, adjust these settings:

```cpp
ServerConfig config;
config.fps = 30;              // Reduce to 15 or 20 for performance
config.max_bitrate_bps = 15000000;  // Reduce to 5000000 for lower quality
```

## Next Steps

After successful testing:

1. Fill in test results in `t1004_test_plan.md`
2. Document any issues found
3. Create a completion report
4. Update task_plan.md with T1004 status

---

**Instructions Created**: 2026-02-15
**Status**: Ready for testing
