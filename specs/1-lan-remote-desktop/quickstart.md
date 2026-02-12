# Quickstart Guide: LAN Low-Latency Remote Desktop SDK

**Feature**: 1-lan-remote-desktop
**Date**: 2025-02-11
**Version**: 1.0.0

## Overview

This guide helps you get started with the Remote Desktop SDK quickly. You'll learn how to:
- Build the SDK from source
- Run the Windows server application
- Connect from a mobile Chrome browser
- Use basic features (screen viewing, mouse/keyboard input, display switching)

## Prerequisites

### Windows Host (Server)

**Operating System**:
- Windows 10 (Version 1903 or later) or Windows 11

**Hardware**:
- GPU with H.264 hardware encoding (NVIDIA NVENC or Intel Quick Sync)
- Minimum 4GB RAM (8GB recommended)
- 1Gbps network interface (100Mbps minimum)

**Software**:
- Visual Studio 2019 or later (C++20 support)
- CMake 3.15 or later
- Python 3.10+ (for WebRTC build)
- Windows SDK 10.0.19041.0 or later

### Mobile Client

**Browser**:
- Google Chrome for Android (Version 90+)
- Google Chrome for iOS (Version 90+)

**Network**:
- Same local network as Windows host
- Stable WiFi connection (5GHz recommended for best performance)

## Installation

### Step 1: Clone Repository

```bash
git clone https://github.com/your-org/RemoteControlSDK.git
cd RemoteControlSDK
```

### Step 2: Install Dependencies

**Windows** (PowerShell):
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg integrate install

# Install dependencies
./vcpkg install spdlog:x64-windows
./vcpkg install nlohmann-json:x64-windows

# Install Python and depot_tools for WebRTC
pip install --upgrade pip
pip install "depot_tools>=0.17.0"
```

### Step 3: Build libwebrtc

```bash
# Set depot_tools in PATH
set PATH=%PATH%;%CD%\depot_tools

# Fetch WebRTC source
mkdir webrtc
cd webrtc
fetch webrtc

# Build for Windows
cd src
gn gen out/Default --args="is_debug=false target_cpu=\"x64\""
ninja -C out/Default
```

**Build Time**: 2-4 hours on modern hardware

### Step 4: Build SDK

```bash
# Return to repository root
cd ../..

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake ^
          -DWEBRTC_ROOT=[path-to-webrtc]

# Build
cmake --build . --config Release

# Build time: 10-20 minutes
```

### Step 5: Install

```bash
# Copy build artifacts to installation directory
cmake --install . --prefix ../install

# Optional: Add install/bin to PATH
set PATH=%PATH%;%CD%\..\install\bin
```

## Quick Start (5 Minutes)

### 1. Start the Server

```bash
cd install/bin
ScreenStreamServer.exe
```

**Expected Output**:
```
[2025-02-11 14:30:00.123] [INFO] ScreenStreamSDK v1.0.0 initializing...
[2025-02-11 14:30:00.234] [INFO] Configuration loaded from default.json
[2025-02-11 14:30:00.345] [INFO] DXGI screen capture initialized
[2025-02-11 14:30:00.456] [INFO] Hardware encoder detected: NVIDIA NVENC
[2025-02-11 14:30:00.567] [INFO] WebRTC transport ready
[2025-02-11 14:30:00.678] [INFO] Server listening on 0.0.0.0:8080
[2025-02-11 14:30:00.789] [INFO] 2 displays detected:
[2025-02-11 14:30:00.890] [INFO]   Display 0: Dell U2419H (1920x1080@60Hz) [PRIMARY]
[2025-02-11 14:30:00.901] [INFO]   Display 1: Dell U2720Q (2560x1440@75Hz)
[2025-02-11 14:30:00.912] [INFO] Server ready. Waiting for connections...
```

**Note the server IP address** (e.g., `192.168.1.100`)

### 2. Connect from Mobile Device

**On Android**:
1. Open Chrome browser
2. Navigate to: `http://<server-ip>:8080`
3. Example: `http://192.168.1.100:8080`
4. Tap "Connect" button

**On iOS**:
1. Open Chrome browser
2. Navigate to: `http://<server-ip>:8080`
3. Tap "Connect" button

**Expected Result**:
- Connection established within 5 seconds
- Desktop screen visible on mobile device
- Latency indicator showing ≤ 30ms

### 3. Control the Desktop

**Mouse Input**:
- Tap anywhere on screen to move cursor
- Tap to click (left mouse button)
- Long-press for right-click

**Keyboard Input**:
- Tap the keyboard icon (bottom right)
- Virtual keyboard appears
- Type to send keystrokes to desktop

**Display Switching**:
- Tap the display icon (top right)
- Select different display from dropdown
- View switches to selected display within 100ms

### 4. Test Gestures

**Zoom**:
- Pinch with two fingers to zoom in/out
- Zoom affects mobile view only (not Windows desktop)

**Pan**:
- Drag with two fingers to pan zoomed view
- Scroll around the desktop

**Swipe**:
- Swipe with one finger to scroll content
- Maps to mouse wheel scroll on desktop

## Basic Usage

### Configuration

Edit `config/default.json` to customize settings:

```json
{
  "server": {
    "bind_address": "0.0.0.0",
    "port": 8080
  },
  "video": {
    "target_fps": 60,
    "max_resolution_width": 1920,
    "max_resolution_height": 1080,
    "default_display": "0",
    "hardware_encoding": true
  },
  "encoding": {
    "gop_size": 1,
    "b_frames": 0,
    "qp_value": 23,
    "max_bitrate_bps": 15000000
  },
  "performance": {
    "max_sessions": 4,
    "input_queue_size": 1000
  },
  "logging": {
    "log_level": "INFO",
    "log_file": "logs/screensdk.log",
    "log_to_console": false
  }
}
```

**Common Settings**:

| Setting | Description | Typical Value |
|---------|-------------|---------------|
| `video.target_fps` | Target frame rate | 30, 60, or 120 |
| `video.default_display` | Default display index | "0", "1", "2", or "3" |
| `encoding.qp_value` | Quality (lower = better) | 18-35 (23 recommended) |
| `encoding.max_bitrate_bps` | Max network bitrate | 5000000-50000000 |
| `performance.max_sessions` | Max concurrent clients | 1-4 |

**After editing**:
```bash
# Restart server to apply changes
ScreenStreamServer.exe
```

### Monitoring

**View Real-Time Metrics**:
- Latency indicator on client UI (ms)
- Network status (WiFi icon)
- Frame rate indicator (fps)
- Connection quality bar (green/yellow/red)

**Server Logs**:
```bash
# View live logs
tail -f logs/screensdk.log

# Search for errors
grep ERROR logs/screensdk.log
```

### Troubleshooting

**Problem**: Connection times out after 5 seconds

**Solution**:
1. Check firewall rules: Allow inbound TCP port 8080
2. Verify devices on same network (same IP subnet)
3. Disable VPN on mobile device
4. Check server logs for specific error message

**Problem**: High latency (> 100ms)

**Solution**:
1. Switch to 5GHz WiFi band
2. Reduce video resolution (edit config: max_resolution_width/height)
3. Check for other devices consuming bandwidth
4. Verify hardware encoding enabled (check server logs)

**Problem**: Display switching takes too long (> 500ms)

**Solution**:
1. Check server logs for display switch errors
2. Verify display ID is valid (0-3)
3. Try lower resolution on second display
4. Restart server

**Problem**: Input not responding

**Solution**:
1. Check latency indicator on client UI
2. Restart connection (tap Disconnect, then Connect)
3. Check server logs for input queue errors
4. Verify server not processing other high-CPU tasks

## Advanced Features

### Multi-Client Support

**Connect multiple devices**:
1. Start server on Windows host
2. Connect Device 1 from mobile Chrome
3. Connect Device 2 from different mobile Chrome
4. Both devices see same desktop
5. Either device can control desktop (FIFO ordering)

**Note**: All clients view the same display. Each client can switch to different displays independently.

### Software Encoding Fallback

**If hardware encoding unavailable**:
1. Server logs warning: "Hardware encoder unavailable, falling back to software encoding"
2. System continues to work with higher CPU usage
3. Expect higher latency (≤ 40ms instead of ≤ 30ms)
4. Consider upgrading GPU for optimal performance

### Automatic Reconnection

**Network interruption**:
1. Client detects disconnection (network loss)
2. Client automatically attempts reconnection
3. Exponential backoff: 1s, 2s, 4s, 8s... up to 60s
4. Connection resumes when network recovers
5. No manual intervention required

**To disable**:
```json
{
  "reconnection": {
    "auto_reconnect": false
  }
}
```

## Performance Tips

**For Best Latency**:
- Use 5GHz WiFi (less interference)
- Keep devices close to WiFi access point
- Disable other bandwidth-consuming activities
- Use wired Ethernet for Windows host (if possible)
- Reduce video resolution if network slow

**For Best Quality**:
- Lower QP value in config (try 18-20)
- Ensure hardware encoding enabled
- Use maximum bitrate (15000000 bps)
- Keep frame rate at 60fps
- Close other GPU-intensive applications

**For Best Stability**:
- Use stable power source (not battery power)
- Avoid sleep/hibernate during sessions
- Monitor logs for memory leaks
- Restart server periodically (24+ hours uptime)

## Next Steps

1. **Explore API**: Read `docs/API.md` for C++ integration
2. **JavaScript Client**: See `web/src/client.js` for customization
3. **Deployment**: Follow `docs/Deployment.md` for production setup
4. **Contribution**: See `CONTRIBUTING.md` for development guidelines

## Support

**Documentation**:
- API Reference: `docs/API.md`
- Architecture: `docs/Architecture.md`
- Performance Tuning: `docs/Performance.md`

**Community**:
- GitHub Issues: https://github.com/your-org/RemoteControlSDK/issues
- Discussions: https://github.com/your-org/RemoteControlSDK/discussions

**Troubleshooting**:
- Check logs: `logs/screensdk.log`
- Enable debug mode: Set `"log_level": "DEBUG"` in config
- Report bugs with logs attached

---

**Version**: 1.0.0 | **Last Updated**: 2025-02-11 | **License**: Apache 2.0
