# Implementation Plan: LAN Low-Latency Remote Desktop SDK

**Branch**: `1-lan-remote-desktop` | **Date**: 2025-02-11 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/1-lan-remote-desktop/spec.md`

## Summary

This feature implements a Windows-based remote desktop SDK that enables mobile devices to view and control a Windows desktop in real-time over a local network. The system consists of a C++ Windows server application and a JavaScript client library for Chrome browsers. Key capabilities include screen capture at 60fps, H.264 hardware encoding, WebRTC-based video transmission with ≤30ms latency, multi-display support (up to 4 displays), touch gesture mapping, and multi-client connections (up to 4 simultaneous users). The architecture follows strict layered separation with event-driven processing, asynchronous non-blocking operations, and comprehensive observability.

## Technical Context

**Language/Version**: C++20 (server), JavaScript ES2020 (client)
**Primary Dependencies**: libwebrtc, DXGI API, NVENC/QuickSync hardware encoders, nlohmann/json (TBD), spdlog (TBD)
**Storage**: Configuration files (JSON), no persistent data storage
**Testing**: Google Test (C++), Jest (JavaScript), manual E2E testing
**Target Platform**: Windows 10/11 (server), Chrome Mobile (client)
**Project Type**: Hybrid - Native application (C++) + Web client (JavaScript)
**Performance Goals**: 60fps @ any resolution (720p, 1080p, 1440p, 4K), ≤30ms end-to-end latency, ≤200MB idle memory, ≤10% idle CPU
**Constraints**: Sub-100ms display switch, 5-second session setup, 24-hour stability without crashes, 4 simultaneous clients
**Scale/Scope**: Single Windows host serving up to 4 mobile clients in local network

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Constitution Principle | Status | Notes |
|---------------------|----------|--------|
| Test-First (NON-NEGOTIABLE) | ✅ Pass | Will enforce TDD with Google Test and Jest |
| SDK Interface Stability | ✅ Pass | Public C++ API will follow SemVer, maintain backward compatibility |
| Real-Time Performance | ✅ Pass | WebRTC architecture, async non-blocking operations, latency instrumentation |
| Cross-Platform Compatibility | ⚠️ N/A | Current scope Windows-only, per spec assumptions |
| Observability & Debugging | ✅ Pass | Structured logging with spdlog, performance metrics collection |
| Layered Architecture | ✅ Pass | 4-layer design (API/Business/Transport/Platform) |
| Event-Driven Model | ✅ Pass | All operations event-driven with callbacks/promises |
| Resource Management | ✅ Pass | Timeout/retry logic, cleanup on disconnect, memory leak prevention |

**Constitution Compliance**: All applicable principles satisfied. Windows-only platform constraint documented in assumptions.

## Project Structure

### Documentation (this feature)

```text
specs/1-lan-remote-desktop/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (to be created)
├── data-model.md        # Phase 1 output (to be created)
├── quickstart.md        # Phase 1 output (to be created)
├── contracts/           # Phase 1 output (to be created)
│   ├── screen_capture.h
│   ├── video_encoder.h
│   ├── webrtc_transport.h
│   ├── input_processor.h
│   └── display_manager.h
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created yet)
```

### Source Code (repository root)

```text
ScreenStreamSDK/
├── include/              # Public API layer (stable)
│   └── screensdk/
│       ├── screensdk.h              # Main SDK entry point
│       ├── config.h                # Configuration API
│       ├── session.h               # Session management API
│       └── events.h               # Event definitions and callbacks
│
├── src/
│   ├── api/                      # Public API Layer
│   │   ├── screensdk.cpp
│   │   ├── config.cpp
│   │   ├── session.cpp
│   │   └── events.cpp
│   │
│   ├── core/                     # Business Logic Layer
│   │   ├── session_manager.cpp      # Multi-client session management
│   │   ├── input_router.cpp       # FIFO input routing
│   │   ├── display_controller.cpp   # Display selection/switching
│   │   └── reconnection_handler.cpp # Exponential backoff reconnection
│   │
│   ├── capture/                  # Screen Capture Module
│   │   ├── dxgi_capture.cpp       # DXGI-based screen capture
│   │   ├── capture_pipeline.cpp     # Frame processing pipeline
│   │   └── display_detector.cpp    # Multi-display enumeration
│   │
│   ├── encoding/                 # Video Encoding Module
│   │   ├── encoder_factory.cpp     # Hardware/software encoder selection
│   │   ├── nvenc_encoder.cpp      # NVIDIA NVENC implementation
│   │   ├── qsv_encoder.cpp       # Intel QuickSync implementation
│   │   ├── software_encoder.cpp    # Software H.264 fallback
│   │   └── encoder_config.cpp     # Low-latency encoding params
│   │
│   ├── transport/                 # Transport Layer (WebRTC abstraction)
│   │   ├── webrtc_manager.cpp      # libwebrtc integration
│   │   ├── video_source.cpp        # Custom video track source
│   │   ├── data_channel.cpp        # Input command channel
│   │   └── signaling_server.cpp    # WebSocket/HTTP signaling
│   │
│   ├── input/                    # Input Processing Module
│   │   ├── input_event.cpp         # Event parsing
│   │   ├── input_mapper.cpp        # Touch to Windows input mapping
│   │   ├── mouse_handler.cpp       # Mouse event handling
│   │   ├── keyboard_handler.cpp    # Keyboard event handling
│   │   └── gesture_handler.cpp    # Gesture recognition
│   │
│   ├── platform/                 # Platform Layer (Windows-specific)
│   │   ├── windows_input.cpp      # SendInput API wrapper
│   │   ├── display_api.cpp       # Display enumeration/change detection
│   │   ├── gpu_detector.cpp       # Hardware capability detection
│   │   └── error_handler.cpp      # Windows error handling
│   │
│   └── utils/
│       ├── logger.cpp              # spdlog wrapper
│       ├── metrics_collector.cpp  # Performance metrics
│       └── thread_pool.cpp        # Async task execution
│
├── web/                         # Client-side JavaScript library
│   ├── src/
│   │   ├── client.js              # Main client class
│   │   ├── webrtc_connection.js  # WebRTC peer connection
│   │   ├── video_renderer.js      # Canvas 2D/WebGL rendering
│   │   ├── input_capture.js       # Touch/mouse/keyboard capture
│   │   ├── gesture_recognizer.js  # Touch gesture detection
│   │   ├── view_controller.js    # Zoom/pan/aspect ratio
│   │   ├── display_selector.js    # Multi-display UI
│   │   └── metrics_display.js    # Latency/quality indicators
│   ├── styles/
│   │   └── client.css            # UI styling
│   ├── index.html                 # Client entry point
│   └── package.json               # NPM dependencies
│
├── tests/
│   ├── unit/                     # Unit tests
│   │   ├── core/
│   │   │   ├── session_manager_test.cpp
│   │   │   ├── input_router_test.cpp
│   │   │   └── display_controller_test.cpp
│   │   ├── capture/
│   │   │   ├── dxgi_capture_test.cpp
│   │   │   └── display_detector_test.cpp
│   │   ├── encoding/
│   │   │   ├── encoder_factory_test.cpp
│   │   │   └── encoder_config_test.cpp
│   │   ├── input/
│   │   │   ├── input_mapper_test.cpp
│   │   │   └── gesture_handler_test.cpp
│   │   └── utils/
│   │       ├── logger_test.cpp
│   │       └── metrics_collector_test.cpp
│   │
│   ├── integration/              # Integration tests
│   │   ├── capture_encoding_test.cpp
│   │   ├── transport_test.cpp
│   │   ├── display_switch_test.cpp
│   │   └── multi_client_test.cpp
│   │
│   ├── e2e/                     # End-to-end tests
│   │   ├── basic_connection_test.cpp
│   │   ├── latency_test.cpp
│   │   ├── multi_display_test.cpp
│   │   └── stability_test.cpp
│   │
│   └── client/                   # JavaScript tests
│       ├── client_test.js
│       ├── gesture_recognizer_test.js
│       └── view_controller_test.js
│
├── config/                     # Configuration files
│   ├── default.json              # Default SDK configuration
│   └── advanced.json             # Advanced options
│
├── docs/                       # Documentation
│   ├── API.md                   # C++ API reference
│   ├── JavaScript-API.md         # JavaScript API reference
│   ├── Architecture.md            # System architecture overview
│   ├── Building.md               # Build instructions
│   ├── Deployment.md             # Deployment guide
│   └── Performance.md            # Performance tuning guide
│
├── third_party/                 # Third-party libraries
│   ├── libwebrtc/             # WebRTC library (TBD evaluation)
│   └── spdlog/                # Logging library (TBD evaluation)
│
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # Project overview
├── LICENSE                     # Apache 2.0 license
├── CHANGELOG.md                # Version history
└── third_party_notices.txt      # Third-party acknowledgments
```

**Structure Decision**: Selected hybrid structure combining native C++ application (ScreenStreamSDK/) with web client (web/). This aligns with spec requirements for Windows server and Chrome browser client. Four-layer architecture enforced: Public API (include/), Business Logic (src/core/), Transport (src/transport/), and Platform (src/platform/). Event-driven model implemented through callback-based API (include/screensdk/events.h). Testing organized by type (unit/integration/e2e/client) with clear separation from production code.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| N/A | N/A | All constitutional requirements satisfied |

## Next Steps

1. **Phase 0 - Research**: Investigate third-party libraries (libwebrtc, spdlog, nlohmann/json) and create `research.md`
2. **Phase 1 - Design**: Define detailed data models (`data-model.md`), API contracts (`contracts/`), and quickstart guide (`quickstart.md`)
3. **Phase 2 - Task Planning**: Run `/speckit.tasks` to generate detailed implementation task breakdown
