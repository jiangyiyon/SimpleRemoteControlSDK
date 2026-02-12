# Architecture Design: LAN Low-Latency Remote Desktop SDK

**Document Version**: 1.0  
**Date**: 2025-02-11  
**Feature**: 1-lan-remote-desktop  
**Status**: Draft

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Design Principles](#design-principles)
3. [Layered Architecture](#layered-architecture)
4. [Component Design](#component-design)
5. [Data Flow](#data-flow)
6. [Concurrency Model](#concurrency-model)
7. [Interface Design](#interface-design)
8. [Error Handling](#error-handling)
9. [Testing Strategy](#testing-strategy)
10. [Deployment Architecture](#deployment-architecture)

---

## Architecture Overview

### High-Level Architecture

The Remote Desktop SDK follows a **Clean Architecture** pattern with four distinct layers:

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                        │
│  (JavaScript Client Library - Chrome Browser)                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Video View   │  │ Input        │  │ UI Controls  │      │
│  │ Controller   │  │ Capture      │  │ & Metrics    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              │
                       WebRTC Protocol
                              │
┌─────────────────────────────────────────────────────────────┐
│                     API Layer (Public)                        │
│  Pure Virtual Interfaces + C-style Factory Functions         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ IScreenSDK   │  │ ISession     │  │ IConfig      │      │
│  │ CreateSDK()  │  │ Create()     │  │ Load()       │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Business Logic Layer                        │
│  Session Management | Input Routing | Display Control        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Session      │  │ Input        │  │ Display      │      │
│  │ Manager      │  │ Router       │  │ Controller   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                    Transport Layer                           │
│  Screen Capture | Video Encoding | WebRTC Transport         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ DXGI         │  │ H.264        │  │ WebRTC       │      │
│  │ Capture      │  │ Encoder      │  │ Manager      │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Platform Layer                              │
│  Windows-specific: GPU, Input, Display APIs                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Windows      │  │ Display      │  │ GPU          │      │
│  │ Input API    │  │ Enumeration  │  │ Detector     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

### Architecture Pattern Selection

**Primary Pattern**: Clean Architecture (Dependency Rule)

**Secondary Patterns**:
- **Hexagonal Architecture**: Ports (interfaces) and Adapters (implementations)
- **Domain-Driven Design**: Bounded contexts (Screen Capture, Input Processing, Session Management)
- **Event-Driven Architecture**: Asynchronous callbacks and event streams

**Rationale**:
1. **ABI Stability**: Pure virtual interfaces enable stable ABI across compiler versions
2. **Testability**: Business logic isolated from frameworks and external dependencies
3. **Maintainability**: Clear separation of concerns with dependency inversion
4. **Performance**: Non-blocking asynchronous operations with minimal lock contention
5. **Cross-Language Compatibility**: `extern "C"` factory functions enable P/Invoke from C#, Python, etc.

---

## Design Principles

### 1. Dependency Rule

**Principle**: Dependencies must always point inward. Inner layers know nothing about outer layers.

**Application**:
- Presentation Layer depends on API Layer
- API Layer depends on Business Logic Layer
- Business Logic Layer depends on Transport Layer
- Transport Layer depends on Platform Layer
- Platform Layer has zero dependencies (uses OS APIs only)

**Benefits**:
- Frameworks and external libraries are pluggable
- Business logic can be tested in isolation
- Easy to swap implementations (e.g., hardware vs software encoder)

### 2. Pure Virtual Interface Pattern

**Principle**: All public APIs are pure virtual structs with C-style factory functions.

**Application**:
```cpp
// include/screensdk/screensdk.h
namespace screensdk {

struct SCREENSDK_EXPORT IScreenSDK {
    virtual ~IScreenSDK() = default;
    
    virtual Result<void> initialize(const Configuration& config) = 0;
    virtual Result<std::string> start() = 0;
    virtual void stop() = 0;
    virtual std::vector<std::shared_ptr<ISession>> getActiveSessions() = 0;
};

extern "C" SCREENSDK_EXPORT IScreenSDK* CreateScreenSDK();
extern "C" SCREENSDK_EXPORT void DestroyScreenSDK(IScreenSDK* sdk);

} // namespace screensdk
```

**Benefits**:
- Stable ABI across DLL boundaries
- Easy to call from C and other languages
- No implementation details exposed in headers
- Version-compatible additions

### 3. Event-Driven Non-Blocking

**Principle**: All I/O operations are asynchronous with callback-based notification.

**Application**:
```cpp
struct ISessionCallback {
    virtual void onConnected(const SessionInfo& info) = 0;
    virtual void onDisconnected(const std::string& reason) = 0;
    virtual void onError(const Error& error) = 0;
    virtual void onLatencyUpdate(int latency_ms) = 0;
};
```

**Benefits**:
- Low-latency response (< 30ms end-to-end)
- No thread blocking on I/O
- Scalable to multiple concurrent sessions

### 4. RAII and Resource Management

**Principle**: All resources are managed through RAII wrappers with deterministic cleanup.

**Application**:
```cpp
class RAIIHandle {
public:
    explicit RAIIHandle(HANDLE h) : handle_(h) {}
    ~RAIIHandle() { if (handle_) CloseHandle(handle_); }
    
    RAIIHandle(const RAIIHandle&) = delete;
    RAIIHandle& operator=(const RAIIHandle&) = delete;
    
    HANDLE get() const { return handle_; }
    
private:
    HANDLE handle_ = nullptr;
};
```

**Benefits**:
- No resource leaks
- Exception-safe cleanup
- Clear ownership semantics

### 5. Thread Safety by Design

**Principle**: Shared data uses atomic operations or lock-free data structures. Minimal locking.

**Application**:
```cpp
class Session {
    std::atomic<int> latency_ms_{0};
    std::atomic<bool> active_{false};
    std::shared_mutex state_mutex_;
};
```

**Benefits**:
- Minimal lock contention
- Scalable to multiple threads
- No deadlocks

---

## System Layering and Module Design

### Layered Architecture Overview

The Remote Desktop SDK follows a strict **five-layer Clean Architecture** with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────────┐
│ Layer 1: Presentation Layer                                   │
│ (JavaScript Client - Chrome Browser)                          │
│ ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│ │ VideoRenderer   │  │ InputCapture    │  │ ViewController  │   │
│ │ GestureRecognizer│ │ DisplaySelector │  │ MetricsDisplay  │   │
│ └────────────────┘  └────────────────┘  └────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                            ↓ WebRTC Protocol
┌─────────────────────────────────────────────────────────────────┐
│ Layer 2: API Layer (Public Interfaces)                          │
│ ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│ │ IScreenSDK     │  │ ISession       │  │ ISDKCallback    │   │
│ │ ISessionCallback│ │ Configuration  │  │ Result<T>       │   │
│ └────────────────┘  └────────────────┘  └────────────────┘   │
│ Factory: CreateScreenSDK(), DestroyScreenSDK()                 │
└─────────────────────────────────────────────────────────────────┘
                            ↓ Depends on
┌─────────────────────────────────────────────────────────────────┐
│ Layer 3: Business Logic Layer                                  │
│ ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│ │ SessionManager │  │ InputRouter    │  │ DisplayController│   │
│ │ ReconnectionHandler│ EventDispatcher │  └────────────────┘   │
│ └────────────────┘  └────────────────┘                       │
└─────────────────────────────────────────────────────────────────┘
                            ↓ Depends on
┌─────────────────────────────────────────────────────────────────┐
│ Layer 4: Transport Layer                                       │
│ ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│ │ IScreenCapture  │  │ IVideoEncoder  │  │ IWebRTCManager   │   │
│ │ IInputProcessor │  └────────────────┘  │ IVideoSource     │   │
│ └────────────────┘                       └────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                            ↓ Depends on
┌─────────────────────────────────────────────────────────────────┐
│ Layer 5: Platform Layer (Windows-Specific)                     │
│ ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│ │ DXGIScreenCapture│ │ NVENCEncoder    │  │ WebRTCManager   │   │
│ │ QSVSEncoder    │  │ SoftwareEncoder │  │ WindowsInputAPI │   │
│ └────────────────┘  └────────────────┘  └────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### Dependency Rules

| Layer | Depends On | Dependencies | Example |
|-------|-----------|--------------|---------|
| Presentation | API Layer | Remote WebRTC connection | JavaScript → IScreenSDK (via P/Invoke) |
| API | Business Logic | Pure interfaces only | IScreenSDK → SessionManager interface |
| Business Logic | Transport | Interface abstractions | SessionManager → IScreenCapture, IVideoEncoder |
| Transport | Platform | OS API wrappers | IVideoEncoder → NVENCEncoder implementation |
| Platform | None | Windows APIs only | Direct DXGI, NVENC, Win32 APIs |

---

### Layer 1: Presentation Layer

**Purpose**: User interface and client-side interaction logic.

**Components**:
- `VideoRenderer`: Canvas/WebGL rendering of video frames
- `InputCapture`: Touch/mouse/keyboard event capture
- `GestureRecognizer`: Pinch-zoom, pan, swipe detection
- `ViewController`: Zoom, pan, aspect ratio management
- `DisplaySelector`: Multi-display UI component
- `MetricsDisplay`: Latency and quality indicators

**Technology Stack**:
- JavaScript ES2020
- HTML5 Canvas / WebGL
- WebRTC MediaStream API
- Chrome DevTools Protocol (optional for debugging)

**Dependencies**: API Layer (via remote WebRTC connection)

---

### Layer 2: API Layer (Public)

**Purpose**: Stable public interface for SDK consumers.

**Interfaces**:

#### IScreenSDK
Main SDK entry point.

```cpp
struct SCREENSDK_EXPORT IScreenSDK {
    virtual ~IScreenSDK() = default;
    
    // Lifecycle
    virtual Result<void> initialize(const Configuration& config) = 0;
    virtual Result<std::string> start(const std::string& bind_address, int port) = 0;
    virtual void stop() = 0;
    
    // Session Management
    virtual Result<std::shared_ptr<ISession>> createSession() = 0;
    virtual std::vector<std::shared_ptr<ISession>> getActiveSessions() = 0;
    
    // Configuration
    virtual Result<void> updateConfig(const Configuration& config) = 0;
    virtual Configuration getConfig() const = 0;
    
    // Observability
    virtual void registerCallback(std::shared_ptr<ISDKCallback> callback) = 0;
    virtual MetricsSnapshot getMetrics() const = 0;
};
```

#### ISession
Individual remote desktop session.

```cpp
struct SCREENSDK_EXPORT ISession {
    virtual ~ISession() = default;
    
    // Connection
    virtual Result<void> connect(const std::string& client_id) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const noexcept = 0;
    
    // Display Control
    virtual std::vector<DisplayInfo> getAvailableDisplays() const = 0;
    virtual Result<void> selectDisplay(int display_id) = 0;
    virtual DisplayInfo getCurrentDisplay() const = 0;
    
    // Input Routing
    virtual void sendInput(const InputEvent& event) = 0;
    
    // Session State
    virtual std::string getSessionId() const = 0;
    virtual SessionState getState() const = 0;
    virtual int getLatency() const = 0;
    
    // Callbacks
    virtual void registerCallback(std::shared_ptr<ISessionCallback> callback) = 0;
};
```

#### ISDKCallback
Global SDK events.

```cpp
struct SCREENSDK_EXPORT ISDKCallback {
    virtual ~ISDKCallback() = default;
    
    virtual void onSessionCreated(const std::string& session_id) = 0;
    virtual void onSessionTerminated(const std::string& session_id, const std::string& reason) = 0;
    virtual void onError(const Error& error) = 0;
};
```

#### ISessionCallback
Session-specific events.

```cpp
struct SCREENSDK_EXPORT ISessionCallback {
    virtual ~ISessionCallback() = default;
    
    virtual void onConnected(const SessionInfo& info) = 0;
    virtual void onDisconnected(const std::string& reason) = 0;
    virtual void onError(const Error& error) = 0;
    virtual void onLatencyUpdate(int latency_ms) = 0;
    virtual void onDisplaySwitched(const DisplayInfo& display) = 0;
};
```

**Factory Functions**:
```cpp
extern "C" SCREENSDK_EXPORT IScreenSDK* CreateScreenSDK();
extern "C" SCREENSDK_EXPORT void DestroyScreenSDK(IScreenSDK* sdk);
```

**Dependencies**: Business Logic Layer (interfaces only)

---

### Layer 3: Business Logic Layer

**Purpose**: Core business rules and orchestration. No external framework dependencies.

#### SessionManager
Manages multiple concurrent sessions with FIFO input routing.

**Responsibilities**:
- Session lifecycle (creation, activation, termination)
- Display switching coordination
- Input event FIFO queue processing
- Reconnection logic with exponential backoff

**Dependencies**:
- Transport Layer (IScreenCapture, IVideoEncoder, IWebRTCManager)
- Platform Layer (IInputInjector, IDisplayManager)

#### InputRouter
Routes input events from clients to Windows desktop.

**Responsibilities**:
- FIFO ordering guarantee for multi-client scenarios
- Input validation (bounds checking, keycode validation)
- Touch gesture to Windows input mapping
- Latency tracking and metrics collection

#### DisplayController
Controls display capture and switching.

**Responsibilities**:
- Display enumeration and change detection
- Display selection and hot-switching (< 100ms)
- Resolution adaptation
- Capture pipeline coordination

#### ReconnectionHandler
Handles automatic reconnection after network interruption.

**Responsibilities**:
- Exponential backoff retry logic
- Connection state restoration
- Graceful degradation on persistent failure

---

### Layer 4: Transport Layer

**Purpose**: Data capture, encoding, and transport. Abstraction over low-level APIs.

#### Screen Capture Module

**Interface**:
```cpp
struct IScreenCapture {
    virtual ~IScreenCapture() = default;
    
    virtual Result<void> initialize(int display_id) = 0;
    virtual Result<void> start() = 0;
    virtual void stop() = 0;
    
    virtual Result<std::shared_ptr<VideoFrame>> captureFrame() = 0;
    virtual std::vector<DisplayInfo> enumerateDisplays() = 0;
};
```

**Implementation**:
- `DXGIScreenCapture`: DXGI Desktop Duplication API

#### Video Encoding Module

**Interface**:
```cpp
struct IVideoEncoder {
    virtual ~IVideoEncoder() = default;
    
    virtual Result<void> initialize(const EncoderConfig& config) = 0;
    virtual Result<void> encode(const VideoFrame& frame, EncodedFrameCallback callback) = 0;
    virtual void release() = 0;
    
    virtual bool supportsHardwareEncoding() const = 0;
};
```

**Implementations**:
- `NVENCEncoder`: NVIDIA NVENC hardware encoder
- `QSVSEncoder`: Intel QuickSync Video encoder
- `SoftwareEncoder`: Fallback software encoder (x264)

#### WebRTC Transport Module

**Interface**:
```cpp
struct IWebRTCManager {
    virtual ~IWebRTCManager() = default;
    
    virtual Result<void> startServer(const std::string& address, int port) = 0;
    virtual void stopServer() = 0;
    
    virtual Result<std::string> createPeerConnection(const std::string& client_id) = 0;
    virtual void closePeerConnection(const std::string& connection_id) = 0;
    
    virtual Result<void> sendVideoTrack(const std::string& connection_id, const std::shared_ptr<IVideoSource> source) = 0;
    virtual Result<void> sendDataChannelMessage(const std::string& connection_id, const std::string& message) = 0;
    
    virtual void registerCallback(std::shared_ptr<IWebRTCCallback> callback) = 0;
};
```

**Implementation**:
- `WebRTCManager`: libwebrtc integration

#### Input Processing Module

**Interface**:
```cpp
struct IInputProcessor {
    virtual ~IInputProcessor() = default;
    
    virtual Result<void> processInput(const InputEvent& event) = 0;
    virtual void registerGestureCallback(std::shared_ptr<IGestureCallback> callback) = 0;
};
```

**Components**:
- `InputMapper`: Touch/mouse/keyboard mapping
- `MouseHandler`: Windows mouse input
- `KeyboardHandler`: Windows keyboard input
- `GestureHandler`: Touch gesture recognition

---

### Layer 5: Platform Layer

**Purpose**: Windows-specific OS API wrappers. Pure implementation, no abstraction.

#### Windows Input API
```cpp
class WindowsInputInjector {
public:
    void sendMouseMove(int x, int y);
    void sendMouseClick(MouseButton button, MouseAction action);
    void sendKeyboardInput(uint8_t keycode, bool pressed);
};
```

#### Display API
```cpp
class DisplayEnumerator {
public:
    std::vector<DisplayInfo> enumerateDisplays();
    bool isDisplayConnected(int display_id);
    Resolution getDisplayResolution(int display_id);
};
```

#### GPU Detector
```cpp
class GPUDetector {
public:
    bool hasNVENC();
    bool hasQuickSync();
    std::string getGPUName();
};
```

---

## Module Design and Class Diagrams

### Module Organization

The system is organized into six core modules, each with clear responsibilities:

```
screensdk/
├── api/                    # Public API Module
│   ├── IScreenSDK          # Main SDK interface
│   ├── ISession            # Session interface
│   ├── ISDKCallback        # Global events
│   ├── ISessionCallback    # Session events
│   └── Configuration       # Configuration data structure
│
├── core/                   # Business Logic Module
│   ├── SessionManager      # Multi-session orchestration
│   ├── InputRouter         # FIFO input routing
│   ├── DisplayController   # Display switching logic
│   └── ReconnectionHandler # Auto-reconnection logic
│
├── capture/                # Screen Capture Module
│   ├── IScreenCapture     # Capture interface
│   ├── DXGIScreenCapture  # DXGI implementation
│   └── CapturePipeline     # Frame processing pipeline
│
├── encoding/               # Video Encoding Module
│   ├── IVideoEncoder      # Encoder interface
│   ├── EncoderFactory      # Factory pattern
│   ├── NVENCEncoder       # NVIDIA implementation
│   ├── QSVSEncoder        # Intel implementation
│   └── SoftwareEncoder    # Fallback implementation
│
├── transport/              # WebRTC Transport Module
│   ├── IWebRTCManager     # WebRTC interface
│   ├── IVideoSource       # Video track source
│   ├── WebRTCManager     # libwebrtc wrapper
│   └── DataChannel        # Input command channel
│
└── input/                  # Input Processing Module
    ├── IInputProcessor    # Processor interface
    ├── InputMapper        # Touch-to-Windows mapping
    ├── MouseHandler       # Mouse input handler
    ├── KeyboardHandler    # Keyboard input handler
    └── GestureHandler     # Gesture recognition
```

---

### Module 1: Public API Module

#### Class Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    api/ Module                             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────┐         ┌─────────────────┐           │
│  │ IScreenSDK      │         │ ISession        │           │
│  ├─────────────────┤         ├─────────────────┤           │
│  │+ initialize()   │         │+ connect()      │           │
│  │+ start()        │◄────────┤+ disconnect()   │           │
│  │+ stop()         │ manages  │+ selectDisplay()│           │
│  │+ createSession()│         │+ sendInput()    │           │
│  │+ getActiveSessions()│     │+ getState()     │           │
│  │+ updateConfig() │         │+ getLatency()   │           │
│  │+ getMetrics()   │         │+ registerCallback()│        │
│  └─────────────────┘         └─────────────────┘           │
│         │                           │                        │
│         │ uses                      │ uses                   │
│         ▼                           ▼                        │
│  ┌─────────────────┐         ┌─────────────────┐           │
│  │ ISDKCallback    │         │ ISessionCallback│           │
│  ├─────────────────┤         ├─────────────────┤           │
│  │+ onSessionCreated() │     │+ onConnected()  │           │
│  │+ onSessionTerminated()│   │+ onDisconnected()│          │
│  │+ onError()      │         │+ onError()      │           │
│  └─────────────────┘         │+ onLatencyUpdate()│          │
│                              │+ onDisplaySwitched()│        │
│                              └─────────────────┘           │
│                                                              │
│  ┌─────────────────┐         ┌─────────────────┐           │
│  │ Configuration   │         │ Result<T>       │           │
│  ├─────────────────┤         ├─────────────────┤           │
│  │ bind_address    │         │ isSuccess()     │           │
│  │ port            │         │ isError()       │           │
│  │ target_fps      │         │ getValue()      │           │
│  │ max_resolution  │         │ getError()      │           │
│  │ hardware_encoding│        │ map()           │           │
│  │ max_sessions    │         └─────────────────┘           │
│  │ ...             │                                       │
│  └─────────────────┘                                       │
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │ Factory Functions (extern "C")             │             │
│  ├────────────────────────────────────────────┤             │
│  │ IScreenSDK* CreateScreenSDK()            │             │
│  │ void DestroyScreenSDK(IScreenSDK*)       │             │
│  └────────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────────┘
```

#### Class Relationships

| Relationship | Description |
|-------------|-------------|
| IScreenSDK manages ISession | Composition, 1-to-many |
| ISession uses ISessionCallback | Association, 1-to-many |
| IScreenSDK uses ISDKCallback | Association, 1-to-many |
| All classes use Configuration | Association, 1-to-many |
| All return types use Result<T> | Composition |

---

### Module 2: Business Logic Module

#### Class Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                   core/ Module                             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │           SessionManager                   │             │
│  ├────────────────────────────────────────────┤             │
│  │ - session_map_: map<string, Session>       │             │
│  │ - input_router_: InputRouter               │             │
│  │ - display_controller_: DisplayController    │             │
│  │ - reconnection_handler_: ReconnectionHandler│           │
│  ├────────────────────────────────────────────┤             │
│  │ + createSession() → Result<Session>        │             │
│  │ + getSession(id) → Session*               │             │
│  │ + terminateSession(id) → Result<void>      │             │
│  │ + broadcastFrame(frame) → void            │             │
│  │ + routeInput(event) → void                │             │
│  └────────────────────────────────────────────┘             │
│          │           │              │                     │
│          │ creates   │ manages       │ manages             │
│          ▼           ▼              ▼                     │
│  ┌────────────┐  ┌──────────┐  ┌──────────────┐          │
│  │  Session   │  │InputRouter│  │DisplayController│       │
│  ├────────────┤  ├──────────┤  ├──────────────┤          │
│  │ session_id │  │event_queue│  │display_map   │          │
│  │ state      │  │          │  │              │          │
│  │ client_ip  │  ├──────────┤  ├──────────────┤          │
│  │ display_id │  │+queue()  │  │+enumerate()  │          │
│  │ latency_ms │  │+process()│  │+select()     │          │
│  │ webrtc_conn│  └──────────┘  │+switch()     │          │
│  ├────────────┤                 │+detectChange()│         │
│  │+connect()  │                 └──────────────┘          │
│  │+disconnect() │                                            │
│  │+sendInput() │  ┌────────────────────┐                  │
│  │+getState()  │  │ ReconnectionHandler │                  │
│  └────────────┘  ├────────────────────┤                  │
│                  │ - retry_count       │                  │
│                  │ - backoff_ms       │                  │
│                  ├────────────────────┤                  │
│                  │+startReconnect()   │                  │
│                  │+stopReconnect()   │                  │
│                  │+handleSuccess()    │                  │
│                  │+handleFailure()    │                  │
│                  └────────────────────┘                  │
│                                                              │
│  Dependencies:                                                │
│  → capture::IScreenCapture                                   │
│  → encoding::IVideoEncoder                                   │
│  → transport::IWebRTCManager                                  │
│  → input::IInputProcessor                                    │
└─────────────────────────────────────────────────────────────┘
```

#### Class Descriptions

**SessionManager**
- Manages multiple concurrent sessions
- Implements session lifecycle (create, activate, terminate)
- Coordinates display switching across sessions
- Routes input events via InputRouter

**Session**
- Represents a single remote desktop connection
- Maintains connection state and metrics
- Delegates video transmission to WebRTCManager
- Emits session-specific events via callbacks

**InputRouter**
- Implements FIFO queue for input events
- Single-threaded processing prevents race conditions
- Validates input bounds and format
- Tracks latency metrics

**DisplayController**
- Enumerates and manages display sources
- Coordinates display switching (< 100ms)
- Detects display hot-plug events
- Maintains capture pipeline state

**ReconnectionHandler**
- Implements exponential backoff retry logic
- Manages reconnection state machine
- Coordinates state restoration with SessionManager
- Emits progress events via callbacks

#### Class Relationships

| Relationship | Description |
|-------------|-------------|
| SessionManager creates Session | Factory, 1-to-many |
| SessionManager owns InputRouter | Composition |
| SessionManager owns DisplayController | Composition |
| SessionManager owns ReconnectionHandler | Composition |
| Session uses IWebRTCManager | Association |
| Session uses IScreenCapture | Association |
| Session uses IVideoEncoder | Association |
| InputRouter uses IInputProcessor | Association |

---

### Module 3: Screen Capture Module

#### Class Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                 capture/ Module                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │            IScreenCapture                  │             │
│  ├────────────────────────────────────────────┤             │
│  │+ initialize(display_id) → Result<void>     │             │
│  │+ start() → Result<void>                   │             │
│  │+ stop() → void                            │             │
│  │+ captureFrame() → Result<VideoFrame>      │             │
│  │+ enumerateDisplays() → vector<DisplayInfo>│            │
│  └────────────────────────────────────────────┘             │
│                          ↑ implements                        │
│                          │                                   │
│  ┌───────────────────────┴──────────────────────┐         │
│  │           DXGIScreenCapture                    │         │
│  ├───────────────────────────────────────────────┤         │
│  │ - duplication_: IDXGIOutputDuplication*      │         │
│  │ - device_: ID3D11Device*                     │         │
│  │ - context_: ID3D11DeviceContext*            │         │
│  │ - display_id_: int                           │         │
│  │ - frame_pool_: ObjectPool<VideoFrame>        │         │
│  ├───────────────────────────────────────────────┤         │
│  │+ initialize(display_id) → Result<void>      │         │
│  │+ start() → Result<void>                      │         │
│  │+ stop() → void                              │         │
│  │+ captureFrame() → Result<VideoFrame>        │         │
│  │+ enumerateDisplays() → vector<DisplayInfo>  │         │
│  └───────────────────────────────────────────────┘         │
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │          CapturePipeline                    │             │
│  ├────────────────────────────────────────────┤             │
│  │ - capturer_: IScreenCapture*              │             │
│  │ - converter_: FormatConverter               │             │
│  │ - frame_queue_: LockFreeQueue<VideoFrame> │            │
│  ├────────────────────────────────────────────┤             │
│  │+ start() → void                           │             │
│  │+ stop() → void                            │             │
│  │+ getFrame() → optional<VideoFrame>        │             │
│  │+ onFrame(callback) → void                 │             │
│  └────────────────────────────────────────────┘             │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │  VideoFrame       │  │   DisplayInfo      │             │
│  ├────────────────────┤  ├────────────────────┤             │
│  │ frame_id          │  │ id                │             │
│  │ timestamp_ns      │  │ name              │             │
│  │ display_source_id │  │ width             │             │
│  │ encoded_data      │  │ height            │             │
│  │ width             │  │ refresh_rate      │             │
│  │ height            │  │ is_primary        │             │
│  │ keyframe          │  └────────────────────┘             │
│  └────────────────────┘                                       │
│                                                              │
│  Dependencies:                                                │
│  → Windows DXGIDesktopDuplication API                         │
│  → Direct3D 11                                                │
└─────────────────────────────────────────────────────────────┘
```

#### Class Descriptions

**IScreenCapture**
- Abstract interface for screen capture
- Platform-agnostic capture API
- Supports multiple display sources
- Frame-by-frame capture with callback support

**DXGIScreenCapture**
- Windows DXGI Desktop Duplication implementation
- Hardware-accelerated screen capture
- Low-latency frame acquisition (< 2ms)
- Automatic display change detection

**CapturePipeline**
- Orchestrates capture, conversion, and queuing
- Runs on dedicated capture thread pool
- Lock-free frame queue for minimal contention
- Object pooling for VideoFrame reuse

**VideoFrame**
- Immutable frame container
- Shared ownership via shared_ptr
- Zero-copy passing between components
- Memory-pooled allocation

#### Class Relationships

| Relationship | Description |
|-------------|-------------|
| CapturePipeline uses IScreenCapture | Association |
| DXGIScreenCapture implements IScreenCapture | Implementation |
| CapturePipeline owns VideoFrame | Composition |
| DXGIScreenCapture creates DisplayInfo | Factory |

---

### Module 4: Video Encoding Module

#### Class Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                encoding/ Module                             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │           IVideoEncoder                     │             │
│  ├────────────────────────────────────────────┤             │
│  │+ initialize(config) → Result<void>         │             │
│  │+ encode(frame, callback) → Result<void>    │             │
│  │+ release() → void                          │             │
│  │+ supportsHardwareEncoding() → bool          │             │
│  └────────────────────────────────────────────┘             │
│                          ↑                                   │
│          ┌───────────────┼───────────────┐                  │
│          │               │               │                  │
│  ┌───────┴──────┐ ┌────┴──────┐ ┌──────┴──────┐           │
│  │ NVENCEncoder  │ │QSVSEncoder│ │SoftwareEncoder│        │
│  ├──────────────┤ ├───────────┤ ├─────────────┤           │
│  │ nvenc_       │ │ qsv_      │ │ x264_       │           │
│  │ encoder_     │ │ session_  │ │ context_    │           │
│  │ input_       │ │           │ │             │           │
│  ├──────────────┤ ├───────────┤ ├─────────────┤           │
│  │+ initialize()│ │+initialize()│ │+initialize() │         │
│  │+ encode()    │ │+encode()   │ │+encode()    │         │
│  │+ release()   │ │+release()  │ │+release()   │         │
│  └──────────────┘ └───────────┘ └─────────────┘           │
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │          EncoderFactory                    │             │
│  ├────────────────────────────────────────────┤             │
│  │ - gpu_detector_: GPUDetector              │             │
│  ├────────────────────────────────────────────┤             │
│  │+ create(config) → unique_ptr<IVideoEncoder>│           │
│  │+ createHardware(config) → ...              │            │
│  │+ createSoftware(config) → ...              │            │
│  │+ detectBestEncoder() → EncoderType         │           │
│  └────────────────────────────────────────────┘             │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │ EncoderConfig      │  │  EncodedFrame      │             │
│  ├────────────────────┤  ├────────────────────┤             │
│  │ codec             │  │ data              │             │
│  │ width             │  │ frame_id          │             │
│  │ height            │  │ timestamp_ns      │             │
│  │ fps               │  │ keyframe          │             │
│  │ bitrate           │  │ size_bytes        │             │
│  │ gop_size          │  │ encoding_time_ms  │             │
│  │ qp_value          │  └────────────────────┘             │
│  │ b_frames          │                                       │
│  │ hardware_enabled  │                                       │
│  └────────────────────┘                                       │
│                                                              │
│  Dependencies:                                                │
│  → NVIDIA NVENC SDK                                           │
│  → Intel Media SDK                                            │
│  → x264 (software fallback)                                   │
└─────────────────────────────────────────────────────────────┘
```

#### Class Descriptions

**IVideoEncoder**
- Abstract encoder interface
- Async encoding with callback notification
- Hardware capability detection
- Resource cleanup on release

**NVENCEncoder**
- NVIDIA NVENC hardware encoder
- H.264/H.265 support
- GPU-based encoding (5-10ms latency)
- Automatic bitrate adaptation

**QSVSEncoder**
- Intel QuickSync Video encoder
- Fixed-function hardware encoder
- Low CPU utilization
- Support for older Intel GPUs

**SoftwareEncoder**
- x264 software encoder fallback
- Cross-platform compatibility
- Higher latency (15-20ms)
- CPU-intensive

**EncoderFactory**
- Factory pattern for encoder selection
- Automatic hardware capability detection
- Graceful degradation (hardware → software)
- Configuration-based selection

#### Class Relationships

| Relationship | Description |
|-------------|-------------|
| EncoderFactory creates IVideoEncoder | Factory |
| NVENCEncoder implements IVideoEncoder | Implementation |
| QSVSEncoder implements IVideoEncoder | Implementation |
| SoftwareEncoder implements IVideoEncoder | Implementation |
| EncoderFactory uses GPUDetector | Association |

---

### Module 5: WebRTC Transport Module

#### Class Diagram

```
┌─────────────────────────────────────────────────────────────┐
│               transport/ Module                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │          IWebRTCManager                   │             │
│  ├────────────────────────────────────────────┤             │
│  │+ startServer(address, port) → Result<void>│           │
│  │+ stopServer() → void                      │             │
│  │+ createPeerConnection(client_id) → Result<string>│      │
│  │+ closePeerConnection(id) → void           │             │
│  │+ sendVideoTrack(id, source) → Result<void>│          │
│  │+ sendDataChannelMessage(id, msg) → Result<void>│       │
│  │+ registerCallback(callback) → void        │             │
│  └────────────────────────────────────────────┘             │
│                          ↑ implements                        │
│                          │                                   │
│  ┌───────────────────────┴──────────────────────┐         │
│  │           WebRTCManager                      │         │
│  ├───────────────────────────────────────────────┤         │
│  │ - peer_factory_: PeerConnectionFactory*       │         │
│  │ - connections_: map<string, PeerConnection>   │         │
│  │ - signaling_server_: SignalingServer        │         │
│  │ - callback_: IWebRTCCallback*               │         │
│  ├───────────────────────────────────────────────┤         │
│  │+ startServer(address, port) → Result<void> │         │
│  │+ stopServer() → void                        │         │
│  │+ createPeerConnection(id) → Result<string>  │         │
│  │+ closePeerConnection(id) → void            │         │
│  │+ sendVideoTrack(id, source) → Result<void> │         │
│  │+ sendDataChannelMessage(id, msg) → ...     │         │
│  └───────────────────────────────────────────────┘         │
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │          IVideoSource                      │             │
│  ├────────────────────────────────────────────┤             │
│  │+ start() → void                           │             │
│  │+ stop() → void                            │             │
│  │+ onFrame(callback) → void                 │             │
│  │+ getFrame() → VideoFrame                  │             │
│  └────────────────────────────────────────────┘             │
│                          ↑                                   │
│                          │                                   │
│  ┌───────────────────────┴──────────────────────┐         │
│  │       CustomVideoTrackSource                  │         │
│  ├───────────────────────────────────────────────┤         │
│  │ - frame_queue_: LockFreeQueue<VideoFrame>     │         │
│  │ - frame_width_: int                          │         │
│  │ - frame_height_: int                         │         │
│  ├───────────────────────────────────────────────┤         │
│  │+ start() → void                             │         │
│  │+ stop() → void                              │             │
│  │+ pushFrame(frame) → void                    │         │
│  │+ onFrame(callback) → void                   │         │
│  └───────────────────────────────────────────────┘         │
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │         DataChannel                        │             │
│  ├────────────────────────────────────────────┤             │
│  │ - channel_: RTCDataChannel*               │             │
│  │ - buffer_: vector<InputEvent>              │             │
│  ├────────────────────────────────────────────┤             │
│  │+ send(message) → Result<void>             │             │
│  │+ onMessage(callback) → void               │             │
│  │+ isOpen() → bool                          │             │
│  └────────────────────────────────────────────┘             │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │ IWebRTCCallback    │  │ SignalingServer   │             │
│  ├────────────────────┤  ├────────────────────┤             │
│  │+ onConnected()    │  │+ start()          │             │
│  │+ onDisconnected() │  │+ stop()           │             │
│  │+ onError()        │  │+ handleOffer()     │             │
│  │+ onIceCandidate() │  │+ handleAnswer()    │             │
│  └────────────────────┘  │+ handleIce()       │             │
│                          └────────────────────┘             │
│                                                              │
│  Dependencies:                                                │
│  → libwebrtc (peerconnection, datachannel)                   │
│  → WebSocket (signaling)                                    │
└─────────────────────────────────────────────────────────────┘
```

#### Class Descriptions

**IWebRTCManager**
- WebRTC peer connection management
- Signaling server integration
- Video track and data channel management
- Connection lifecycle control

**WebRTCManager**
- libwebrtc wrapper implementation
- Manages multiple peer connections
- Handles ICE/STUN/TURN negotiation
- Emits connection state events

**IVideoSource**
- Custom video track source interface
- Provides frames to WebRTC
- Callback-based frame delivery
- Lifecycle management

**CustomVideoTrackSource**
- Implements cricket::VideoCapturer
- Lock-free frame queue
- Frame rate throttling
- Format conversion (NV12 → I420)

**DataChannel**
- WebRTC data channel wrapper
- Input event serialization
- Ordered reliable delivery
- Buffer management

#### Class Relationships

| Relationship | Description |
|-------------|-------------|
| WebRTCManager implements IWebRTCManager | Implementation |
| WebRTCManager creates PeerConnection | Factory |
| CustomVideoTrackSource implements IVideoSource | Implementation |
| WebRTCManager uses DataChannel | Composition |
| WebRTCManager uses SignalingServer | Association |

---

### Module 6: Input Processing Module

#### Class Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                 input/ Module                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────────────────────────────┐             │
│  │         IInputProcessor                   │             │
│  ├────────────────────────────────────────────┤             │
│  │+ processInput(event) → Result<void>      │             │
│  │+ registerGestureCallback(cb) → void      │             │
│  └────────────────────────────────────────────┘             │
│                          ↑                                   │
│                          │                                   │
│  ┌───────────────────────┴──────────────────────┐         │
│  │          InputProcessor                     │         │
│  ├───────────────────────────────────────────────┤         │
│  │ - mouse_handler_: MouseHandler              │         │
│  │ - keyboard_handler_: KeyboardHandler        │         │
│  │ - gesture_handler_: GestureHandler          │         │
│  │ - input_mapper_: InputMapper                │         │
│  ├───────────────────────────────────────────────┤         │
│  │+ processInput(event) → Result<void>         │             │
│  │+ registerGestureCallback(cb) → void       │         │
│  └───────────────────────────────────────────────┘         │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │  MouseHandler      │  │ KeyboardHandler    │             │
│  ├────────────────────┤  ├────────────────────┤             │
│  │+ sendMouseMove()   │  │+ sendKeyPress()    │             │
│  │+ sendMouseClick()  │  │+ sendKeyRelease()  │             │
│  │+ sendMouseDrag()   │  │+ sendKeyCombo()    │             │
│  └────────────────────┘  └────────────────────┘             │
│           │                      │                            │
│           │ uses                 │ uses                       │
│           ▼                      ▼                            │
│  ┌────────────────────────────────────────────┐             │
│  │        WindowsInputAPI                     │             │
│  ├────────────────────────────────────────────┤             │
│  │+ sendMouseMove(x, y) → void               │             │
│  │+ sendMouseClick(button, action) → void     │             │
│  │+ sendKeyboardInput(keycode, pressed) → void│            │
│  └────────────────────────────────────────────┘             │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │   InputMapper      │  │  GestureHandler   │             │
│  ├────────────────────┤  ├────────────────────┤             │
│  │+ touchToMouse()   │  │+ handleTouch()     │             │
│  │+ touchToKeyboard()│  │+ handlePinch()     │             │
│  │+ normalizeCoords()│  │+ handlePan()       │             │
│  │+ gestureToInput() │  │+ handleLongPress()  │             │
│  └────────────────────┘  │+ handleSwipe()     │             │
│                          └────────────────────┘             │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐             │
│  │   InputEvent       │  │  GestureEvent     │             │
│  ├────────────────────┤  ├────────────────────┤             │
│  │ event_id          │  │ gesture_type       │             │
│  │ event_type        │  │ scale             │             │
│  │ timestamp_ns      │  │ delta_x           │             │
│  │ client_id         │  │ delta_y           │             │
│  │ payload           │  │ duration_ms       │             │
│  └────────────────────┘  └────────────────────┘             │
│                                                              │
│  Dependencies:                                                │
│  → Windows SendInput API                                      │
│  → libwebrtc data channel                                     │
└─────────────────────────────────────────────────────────────┘
```

#### Class Descriptions

**IInputProcessor**
- Abstract input processing interface
- Event routing and validation
- Gesture callback registration

**InputProcessor**
- Orchestrates mouse, keyboard, and gesture handlers
- Coordinates input mapping
- Validates input bounds
- Emits processed input events

**MouseHandler**
- Windows mouse input simulation
- Absolute and relative positioning
- Click and drag support
- Button state management

**KeyboardHandler**
- Windows keyboard input simulation
- Key press/release handling
- Modifier key support (Ctrl, Alt, Shift)
- Key combination sequences

**InputMapper**
- Touch-to-mouse/keyboard translation
- Coordinate normalization (mobile screen → desktop)
- Gesture-to-input conversion
- Device-specific mapping rules

**GestureHandler**
- Multi-touch gesture recognition
- Pinch-zoom, pan, swipe, long-press detection
- Threshold-based classification
- Client-side gesture handling only

#### Class Relationships

| Relationship | Description |
|-------------|-------------|
| InputProcessor implements IInputProcessor | Implementation |
| InputProcessor owns MouseHandler | Composition |
| InputProcessor owns KeyboardHandler | Composition |
| InputProcessor owns GestureHandler | Composition |
| InputProcessor owns InputMapper | Composition |
| MouseHandler uses WindowsInputAPI | Association |
| KeyboardHandler uses WindowsInputAPI | Association |

---

## Data Flow

### Video Capture and Transmission Flow

```
1. Display Controller starts capture on Display 0
   └─> IDXGIOutputDuplication::AcquireNextFrame()

2. Capture pipeline processes frame
   └─> DXGI surface conversion to NV12 format

3. Hardware encoder receives frame
   └─> NVENC NvEncEncodePicture()

4. Encoded frame queued for WebRTC transmission
   └─> Lock-free queue (size = 3)

5. WebRTC video track source reads frame
   └─> cricket::VideoCapturer::OnFrame()

6. WebRTC PeerConnection transmits via RTP
   └─> H.264 payload, RTCP feedback

7. Client Chrome browser receives RTP
   └─> WebRTC MediaStreamTrack

8. Canvas/WebGL renderer decodes and displays
   └─> HTMLVideoElement or WebGL texture
```

**Latency Breakdown**:
- Capture: 1-2ms (DXGI)
- Encoding: 5-10ms (NVENC/QuickSync)
- Queue/Transmission: 5-10ms (WebRTC)
- Decoding: 5-8ms (browser hardware decoder)
- Total: **16-30ms** ✓

### Input Event Flow

```
1. Mobile user performs touch action
   └─> touchstart/touchmove/touchend events

2. Input capture handler records event
   └─> Event timestamp and coordinates

3. Gesture recognizer classifies action
   └─> Pinch-zoom, pan, tap, long-press

4. Input mapper converts to Windows input
   └─> SendInput API preparation

5. JSON serialization for WebRTC data channel
   └─> {"type":"mouse_click","x":1920,"y":1080,...}

6. WebRTC data channel transmission
   └─> SCTP, ordered delivery

7. Windows host receives and parses
   └─> InputRouter::processInput()

8. FIFO queue processing (multi-client)
   └─> Single-threaded event loop

9. Input injector executes SendInput
   └─> Windows input subsystem

10. Desktop responds
    └─> Screen change captured in next frame
```

**Latency Breakdown**:
- Touch capture: 1-2ms
- Gesture recognition: 1-2ms
- JSON serialization: <1ms
- WebRTC transmission: 5-8ms
- Processing/injection: 2-3ms
- Next frame capture: 16ms (60fps)
- Total: **25-32ms** ✓

---

## Concurrency Model

### Thread Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  Main Thread (Single)                   │
│  WebRTC Signaling | Session Manager | Event Loop        │
└─────────────────────────────────────────────────────────┘
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
┌───────▼──────┐  ┌──────▼───────┐  ┌─────▼──────┐
│ Capture      │  │ Encoder      │  │ I/O         │
│ Thread Pool  │  │ Thread Pool  │  │ Thread Pool │
│ (2-4 threads)│  │ (2-4 threads)│  │ (2 threads) │
└──────────────┘  └──────────────┘  └─────────────┘
```

### Thread Responsibilities

**Main Thread**:
- WebRTC signaling (SDP offer/answer exchange)
- Session lifecycle management
- Input event dispatch
- Metrics aggregation

**Capture Thread Pool**:
- DXGI frame acquisition
- Surface format conversion
- Display hot-plug detection

**Encoder Thread Pool**:
- Hardware encoding (NVENC/QuickSync)
- Software encoding fallback
- Bitrate adaptation

**I/O Thread Pool**:
- WebSocket signaling server
- HTTP file serving
- Log file writing

### Synchronization Strategy

**Lock-Free Data Structures**:
- Video frame queue (lock-free ring buffer)
- Input event queue (lock-free ring buffer)
- Metrics counters (std::atomic)

**Mutex-Protected Data**:
- Session map (std::shared_mutex for read-heavy access)
- Configuration cache (std::mutex, write-once-after-init)

**Event-Driven Coordination**:
- Condition variables for frame availability
- Callback-based notification for async operations

### Thread Safety Guarantees

**Read-Only Data**: No synchronization needed
- Configuration (after init)
- Display enumeration cache
- Static lookups

**Atomic Operations**: No locks needed
- Latency counters (std::atomic<int>)
- Active flags (std::atomic<bool>)
- Queue indices (std::atomic<size_t>)

**Producer-Consumer**: Queue-based coordination
- Video frames: Capture → Encoder → WebRTC
- Input events: WebRTC → Router → Injector

---

## Interface Design

### Pure Virtual Interface Pattern Compliance

All public interfaces follow the pure virtual interface pattern as defined in the PureVirtualInterfacePattern rule.

### Example: IScreenSDK Interface

```cpp
// include/screensdk/screensdk.h
#pragma once

#include "types.h"
#include "result.h"
#include "config.h"
#include "session.h"
#include "../export.h"

#include <string>
#include <vector>
#include <memory>

namespace screensdk {

struct SCREENSDK_EXPORT IScreenSDK {
    virtual ~IScreenSDK() = default;
    
    // Lifecycle
    virtual Result<void> initialize(const Configuration& config) = 0;
    virtual Result<std::string> start(const std::string& bind_address, int port) = 0;
    virtual void stop() = 0;
    
    // Session Management
    virtual Result<std::shared_ptr<ISession>> createSession() = 0;
    virtual std::vector<std::shared_ptr<ISession>> getActiveSessions() = 0;
    
    // Configuration
    virtual Result<void> updateConfig(const Configuration& config) = 0;
    virtual Configuration getConfig() const = 0;
    
    // Observability
    virtual void registerCallback(std::shared_ptr<ISDKCallback> callback) = 0;
    virtual MetricsSnapshot getMetrics() const = 0;
};

extern "C" SCREENSDK_EXPORT IScreenSDK* CreateScreenSDK();
extern "C" SCREENSDK_EXPORT void DestroyScreenSDK(IScreenSDK* sdk);

} // namespace screensdk
```

### Example: Implementation Class

```cpp
// src/api/screensdk_impl.cpp
#include "screensdk/screensdk.h"

namespace screensdk {

class ScreenSDKImpl : public IScreenSDK {
public:
    ScreenSDKImpl();
    ~ScreenSDKImpl() override;
    
    ScreenSDKImpl(const ScreenSDKImpl&) = delete;
    ScreenSDKImpl& operator=(const ScreenSDKImpl&) = delete;
    ScreenSDKImpl(ScreenSDKImpl&&) = delete;
    ScreenSDKImpl& operator=(ScreenSDKImpl&&) = delete;
    
    Result<void> initialize(const Configuration& config) override;
    Result<std::string> start(const std::string& bind_address, int port) override;
    void stop() override;
    
    Result<std::shared_ptr<ISession>> createSession() override;
    std::vector<std::shared_ptr<ISession>> getActiveSessions() override;
    
    Result<void> updateConfig(const Configuration& config) override;
    Configuration getConfig() const override;
    
    void registerCallback(std::shared_ptr<ISDKCallback> callback) override;
    MetricsSnapshot getMetrics() const override;
    
private:
    std::unique_ptr<SessionManager> session_manager_;
    std::shared_ptr<ISDKCallback> callback_;
    Configuration config_;
    std::atomic<bool> running_{false};
};

} // namespace screensdk

extern "C" screensdk::IScreenSDK* CreateScreenSDK() {
    return new screensdk::ScreenSDKImpl();
}

extern "C" void DestroyScreenSDK(screensdk::IScreenSDK* sdk) {
    if (sdk != nullptr) {
        delete sdk;
    }
}
```

---

## Error Handling

### Error Handling Strategy

**Principle**: Use Result<T> type for error propagation. No exceptions for normal control flow.

### Result Type Design

```cpp
// include/screensdk/result.h
#pragma once

#include <variant>
#include <string>
#include <optional>

namespace screensdk {

template<typename T>
class Result {
public:
    // Success constructor
    static Result<T> success(T value) {
        return Result<T>(std::move(value));
    }
    
    // Error constructor
    static Result<T> error(const std::string& message) {
        return Result<T>(Error{message});
    }
    
    bool isSuccess() const noexcept {
        return std::holds_alternative<T>(value_);
    }
    
    bool isError() const noexcept {
        return std::holds_alternative<Error>(value_);
    }
    
    const T& getValue() const {
        return std::get<T>(value_);
    }
    
    const std::string& getError() const {
        return std::get<Error>(value_).message;
    }
    
    // Monadic operations
    template<typename F>
    auto map(F&& f) -> Result<decltype(f(std::declval<T>()))> {
        if (isSuccess()) {
            return Result<decltype(f(std::declval<T>()))>::success(f(getValue()));
        } else {
            return Result<decltype(f(std::declval<T>()))>::error(getError());
        }
    }
    
private:
    struct Error {
        std::string message;
    };
    
    std::variant<T, Error> value_;
};

// Specialization for void
template<>
class Result<void> {
public:
    static Result<void> success() {
        return Result<void>();
    }
    
    static Result<void> error(const std::string& message) {
        return Result<void>(message);
    }
    
    bool isSuccess() const noexcept {
        return error_message_.empty();
    }
    
    bool isError() const noexcept {
        return !error_message_.empty();
    }
    
    const std::string& getError() const {
        return error_message_;
    }
    
private:
    Result() = default;
    explicit Result(const std::string& message) : error_message_(message) {}
    
    std::string error_message_;
};

} // namespace screensdk
```

### Error Categories

**Recoverable Errors**:
- Network timeout (auto-retry)
- Display disconnected (try re-enumeration)
- Encoder busy (queue and retry)
- Temporary resource unavailability

**Unrecoverable Errors**:
- Invalid configuration (immediate stop)
- Out of memory (immediate stop)
- Critical OS failure (immediate stop)
- Hardware encoder unavailable (fallback to software)

### Error Recovery Strategies

**Exponential Backoff (Network)**:
```cpp
int backoff_ms = 100;
int max_backoff_ms = 60000;

while (!connected && retry_count < MAX_RETRIES) {
    auto result = attemptConnect();
    if (result.isSuccess()) break;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
    backoff_ms = std::min(backoff_ms * 2, max_backoff_ms);
    retry_count++;
}
```

**Graceful Degradation (Encoding)**:
```cpp
auto encoder = EncoderFactory::create(config);
if (!encoder->supportsHardwareEncoding()) {
    LOG_WARN("Hardware encoding unavailable, using software encoder");
    encoder = EncoderFactory::createSoftware(config);
}
```

**Resource Exhaustion (Frame Queue)**:
```cpp
if (frame_queue.isFull()) {
    // Drop oldest frame, queue new one
    frame_queue.popOldest();
    metrics.dropped_frames++;
}
frame_queue.push(frame);
```

---

## Testing Strategy

### Test Architecture

**Test Pyramid**:
```
              ┌─────────┐
              │   E2E   │  (5% - Critical user journeys)
              ├─────────┤
            ┌─────────┐
            │Integration│ (15% - Component interactions)
            ├─────────┤
          ┌───────────┐
          │   Unit    │  (80% - Business logic)
          └───────────┘
```

### Unit Tests

**Scope**: Individual functions and classes in isolation.

**Framework**: Google Test

**Examples**:

```cpp
// tests/unit/core/session_manager_test.cpp
TEST(SessionManagerTest, CreateSessionReturnsValidSession) {
    auto manager = SessionManager::create();
    auto result = manager->createSession();
    
    ASSERT_TRUE(result.isSuccess());
    auto session = result.getValue();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->getState(), SessionState::DISCONNECTED);
}

TEST(InputRouterTest, FIFOOrderingMultiClient) {
    auto router = InputRouter::create();
    
    // Client 1 sends input
    router->queueInput("client1", InputEvent{1, InputEventType::MOUSE_CLICK});
    // Client 2 sends input
    router->queueInput("client2", InputEvent{2, InputEventType::MOUSE_CLICK});
    
    auto processed = router->processBatch();
    ASSERT_EQ(processed.size(), 2);
    EXPECT_EQ(processed[0].event_id, 1);
    EXPECT_EQ(processed[1].event_id, 2);
}
```

**Coverage Goals**:
- Core business logic: 95%
- Transport layer: 90%
- Platform layer: 85%

### Integration Tests

**Scope**: Interactions between components.

**Examples**:

```cpp
// tests/integration/capture_encoding_test.cpp
TEST(CaptureEncodingIntegration, EndToEndEncoding) {
    auto capture = DXGIScreenCapture::create();
    capture->initialize(0);
    
    auto encoder = EncoderFactory::createHardware();
    encoder->initialize(EncoderConfig{});
    
    auto frame = capture->captureFrame().getValue();
    bool encoding_complete = false;
    
    encoder->encode(frame, [&](const EncodedFrame& encoded) {
        encoding_complete = true;
        EXPECT_GT(encoded.data.size(), 0);
        EXPECT_TRUE(encoded.keyframe || encoded.frame_id > 0);
    });
    
    ASSERT_TRUE(encoding_complete);
}
```

### End-to-End Tests

**Scope**: Full system from client connection to input execution.

**Framework**: Google Test + Mock WebRTC

**Test Scenarios**:

1. **Basic Connection**:
   - Client connects → WebRTC handshake → Video stream starts → Client sees desktop

2. **Input Latency**:
   - Client sends click → Measure round-trip time → Assert < 30ms

3. **Display Switching**:
   - Switch from Display 0 to Display 1 → Measure switch time → Assert < 100ms

4. **Multi-Client**:
   - 3 clients connect → All receive video → All can send input

5. **Reconnection**:
   - Disconnect network → Auto-reconnect → Verify state restoration

### JavaScript Client Tests

**Framework**: Jest

**Examples**:

```javascript
// tests/client/gesture_recognizer_test.js
describe('GestureRecognizer', () => {
  test('recognizes pinch-zoom gesture', () => {
    const recognizer = new GestureRecognizer();
    
    recognizer.handleTouchStart([
      {x: 100, y: 100, id: 1},
      {x: 200, y: 100, id: 2}
    ]);
    
    recognizer.handleTouchMove([
      {x: 80, y: 100, id: 1},
      {x: 220, y: 100, id: 2}
    ]);
    
    const gesture = recognizer.getGesture();
    expect(gesture.type).toBe('PINCH_ZOOM');
    expect(gesture.scale).toBeCloseTo(1.5);
  });
});
```

---

## Deployment Architecture

### Deployment Model

**Server (Windows Host)**:
- Single executable: `screensdk_server.exe`
- Dynamic library: `screensdk.dll` (public API)
- Configuration: `config/default.json`
- Logs: `logs/screensdk.log`

**Client (Mobile Chrome)**:
- Single HTML file: `index.html`
- JavaScript bundle: `client.js`
- CSS: `client.css`

### Installation

**Server**:
```bash
# Copy files
copy screensdk_server.exe C:\Program Files\ScreenSDK\
copy screensdk.dll C:\Program Files\ScreenSDK\
copy config\* C:\Program Files\ScreenSDK\config\

# Register Windows service (optional)
screensdk_server.exe --install-service
```

**Client**:
```bash
# Serve static files via HTTP server (for development)
cd web
python -m http.server 8080

# For production, deploy to any web server
```

### Network Architecture

```
┌─────────────────────────────────────────────────────┐
│                   Local Area Network                 │
│                                                      │
│  ┌──────────────┐         ┌──────────────┐         │
│  │  Windows     │         │  Mobile      │         │
│  │  Host        │◄────────┤  Chrome      │         │
│  │ 192.168.1.100│  WebRTC │ 192.168.1.101│         │
│  │  Port 8080   │         │  Port N/A    │         │
│  └──────────────┘         └──────────────┘         │
│                                                      │
│  ┌──────────────┐         ┌──────────────┐         │
│  │  Mobile      │         │  Mobile      │         │
│  │  Chrome      │         │  Chrome      │         │
│  │ 192.168.1.102│         │ 192.168.1.103│         │
│  └──────────────┘         └──────────────┘         │
└─────────────────────────────────────────────────────┘
```

### Firewall Configuration

**Required Ports**:
- TCP 8080: WebRTC signaling (WebSocket)
- UDP 50000-59999: WebRTC media transport (range)

**Windows Firewall Rules**:
```powershell
New-NetFirewallRule -DisplayName "ScreenSDK Server" `
    -Direction Inbound -Protocol TCP -LocalPort 8080 `
    -Action Allow

New-NetFirewallRule -DisplayName "ScreenSDK WebRTC Media" `
    -Direction Inbound -Protocol UDP -LocalPort 50000-59999 `
    -Action Allow
```

### Performance Tuning

**Hardware Requirements**:
- CPU: Intel i5 / AMD Ryzen 5 or better
- GPU: NVIDIA GTX 1650 or Intel Arc A380 (for H.264 hardware encoding)
- RAM: 8GB minimum
- Network: Gigabit LAN

**Software Configuration**:
- Hardware encoding: Enabled (default)
- Target FPS: 60
- Max resolution: 1920x1080
- Max bitrate: 15Mbps
- GOP size: 1 (low-latency mode)
- B-frames: 0 (no reordering)

**Monitoring**:
- Latency metrics (target: < 30ms)
- Frame rate (target: 60fps)
- Packet loss (target: < 5%)
- CPU usage (target: < 30%)
- Memory usage (target: < 200MB idle)

---

## Appendix A: Architecture Decisions Record

### ADR-001: Pure Virtual Interface Pattern for Public API

**Status**: Accepted

**Context**: SDK needs stable ABI across versions and cross-language compatibility.

**Decision**: All public interfaces use pure virtual structs with `extern "C"` factory functions.

**Consequences**:
- + Stable ABI across compiler versions
- + C# / Python interop via P/Invoke
- + No implementation details exposed
- - Slightly more verbose than headers with inline implementations
- - Requires manual memory management (factory/destroy pattern)

### ADR-002: WebRTC for Video Transport

**Status**: Accepted

**Context**: Need real-time video transmission from Windows to Chrome browser.

**Decision**: Use WebRTC (libwebrtc) for video transport protocol.

**Alternatives Considered**:
- RTSP: Too high latency (100-500ms)
- WebSocket video streaming: No browser-native decoding
- Custom UDP: Too complex, requires codec negotiation

**Consequences**:
- + Native browser H.264 decoding
- + Built-in congestion control
- + ICE/STUN/TURN support (for NAT traversal)
- - Larger dependency (libwebrtc)
- - Complexity in signaling setup

### ADR-003: Lock-Free Queues for Performance

**Status**: Accepted

**Context**: Need high-throughput frame and input event passing with minimal latency.

**Decision**: Use lock-free ring buffers for video frame and input event queues.

**Alternatives Considered**:
- std::mutex + std::queue: Too much lock contention
- Boost lockfree: External dependency
- Semaphore-based: Complex API

**Consequences**:
- + Minimal lock contention
- + Deterministic performance
- + No external dependencies
- - More complex implementation
- - Harder to debug concurrency issues

### ADR-004: Result<T> Instead of Exceptions

**Status**: Accepted

**Context**: Need error handling strategy that doesn't impact performance.

**Decision**: Use Result<T> monadic type for error propagation. No exceptions.

**Alternatives Considered**:
- Exceptions: Too much overhead for hot paths
- Error codes: C-style, unergonomic
- std::optional: No error message

**Consequences**:
- + Zero-cost abstraction
- + Functional programming style (map, flatMap)
- + Explicit error handling
- - Verbose error checking
- - No automatic stack unwinding

---

## Appendix B: Terminology

| Term | Definition |
|------|------------|
| **ABI** | Application Binary Interface - stable binary contract between DLL and consumers |
| **PeerConnection** | WebRTC peer-to-peer connection object |
| **SDP** | Session Description Protocol - media negotiation format |
| **ICE** | Interactive Connectivity Establishment - NAT traversal protocol |
| **RTT** | Round-Trip Time - network latency measurement |
| **GOP** | Group of Pictures - video frame sequence between keyframes |
| **IDR** | Instantaneous Decoder Refresh - H.264 keyframe |
| **DXGI** | DirectX Graphics Infrastructure - Windows screen capture API |
| **NVENC** | NVIDIA Encoder - hardware video encoding |
| **QuickSync** | Intel Quick Sync Video - hardware video encoding |
| **SCTP** | Stream Control Transmission Protocol - WebRTC data channel transport |

---

## References

1. [Clean Architecture - Robert C. Martin](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html)
2. [Hexagonal Architecture - Alistair Cockburn](https://alistair.cockburn.us/hexagonal-architecture/)
3. [WebRTC Documentation](https://webrtc.org/)
4. [DXGI Desktop Duplication API](https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api)
5. [NVIDIA NVENC API](https://developer.nvidia.com/nvidia-video-codec-sdk)
6. [Intel Quick Sync Video](https://www.intel.com/content/www/us/en/docs/oneapi/programming-guide/2023-3/intel-media-sdk-hardware-acceleration.html)
7. [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
8. [Herb Sutter's C++ Coding Standards](https://herbsutter.com/elements-of-modern-c-style/)

---

**End of Document**
