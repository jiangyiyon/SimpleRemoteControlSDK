# RemoteControlSDK

Low-latency remote desktop SDK for Windows, enabling mobile device control via WebRTC.

## Features

- **Real-time screen capture**: 60 FPS at 1920x1080 resolution
- **Hardware encoding**: NVIDIA NVENC / Intel QuickSync support with x264 software fallback
- **Low latency**: ≤30ms one-way input-to-display latency on LAN (<20ms RTT)
- **Multi-display support**: Seamless display switching within 100ms
- **Touch gestures**: Pinch-zoom, pan, long-press, swipe support
- **Multi-client**: Up to 4 simultaneous connections with FIFO input ordering
- **Error handling**: Structured error types with detailed messages

## Project Structure

```
RemoteControlSDK/
├── ScreenStreamSDK/     # C++ server SDK
│   ├── src/              # Source code
│   ├── include/screensdk/ # Public headers
│   ├── tests/            # C++ unit tests
│   └── CMakeLists.txt
├── web/                 # JavaScript web client
│   ├── src/
│   ├── tests/
│   └── package.json
├── tests/               # Integration/E2E tests
├── docs/                # Documentation
├── config/              # Configuration files
│   └── default.json
└── third_party/          # Third-party libraries
    ├── webrtc/
    ├── x264/
    └── nlohmann/
```

## Requirements

- **Host**: Windows 10 or 11
- **Hardware**: GPU with H.264 encoding capability (NVIDIA NVENC or Intel QuickSync)
- **Network**: Same local network with <20ms RTT
- **Bandwidth**: ~5-15 Mbps for 1080p@60fps
- **Client**: Chrome browser with WebRTC and H.264 support

## Build Requirements

- **CMake**: 3.15 or higher
- **C++**: C++20 support
- **x264**: Software H.264 encoder (via vcpkg or pre-built)
- **Node.js**: 18+ (for web client tests)
- **Jest**: 29+ (for testing framework)

## Quick Start

### Server (Windows)

```bash
# Build C++ SDK
cd ScreenStreamSDK
mkdir build && cd build
cmake ..
cmake --build . --config Release
cmake --build . --target install

# Run server
cd ../bin
ScreenStreamSDK.exe
```

### Client (Web)

```bash
cd web
npm install
npm start
```

## Configuration

Edit `config/default.json` to customize:

- `encoding.b_frames`: B-frame count (0-10, default: 3)
- `encoding.gop_size`: I-frame interval (default: 60)
- `server.max_clients`: Maximum concurrent clients (default: 4)
- `encoding.bitrate`: Video bitrate in bps (default: 5000000)

## License

Apache 2.0 License

See LICENSE file for details.
