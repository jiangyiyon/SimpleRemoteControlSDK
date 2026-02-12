# Contracts: LAN Low-Latency Remote Desktop SDK

**Feature**: 1-lan-remote-desktop
**Date**: 2025-02-11
**Status**: Draft

## Overview

This document defines the API contracts for the Remote Desktop SDK. All contracts specify public interfaces, data structures, and behavioral guarantees. These serve as the foundation for implementation and test development.

---

## Contract 1: Screen Capture (screen_capture.h)

### Interface: IScreenCapture

**Purpose**: Abstract screen capture interface supporting multiple displays with hardware acceleration detection.

**Methods**:

```cpp
class IScreenCapture {
public:
    virtual ~IScreenCapture() = default;

    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // Display enumeration
    virtual std::vector<DisplaySource> enumerateDisplays() = 0;
    virtual DisplaySource getPrimaryDisplay() = 0;

    // Capture control
    virtual bool startCapture(const DisplaySource& display) = 0;
    virtual void stopCapture() = 0;
    virtual DisplaySource getCurrentDisplay() const = 0;

    // Frame acquisition (non-blocking)
    virtual std::shared_ptr<VideoFrame> getNextFrame(uint32_t timeout_ms) = 0;

    // Capability queries
    virtual bool supportsHardwareEncoding() const = 0;
    virtual Resolution getNativeResolution(const DisplaySource& display) const = 0;

    // Event callbacks
    virtual void setDisplayChangeCallback(std::function<void()> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};
```

**Behavioral Guarantees**:
- `getNextFrame()` must be thread-safe and non-blocking (timeout_ms = 0 for immediate return)
- Display enumeration must complete within 500ms
- Frame capture must maintain 60fps minimum (16.67ms per frame average)
- Memory allocation per frame ≤ 4MB (1080p@60fps worst case)
- Thread safety: `getNextFrame()` can be called from multiple threads

**Error Handling**:
- Throws `std::runtime_error` if DXGI initialization fails
- Returns nullptr from `getNextFrame()` if capture stopped or timeout
- Calls error callback if duplicate output fails during capture

---

## Contract 2: Video Encoder (video_encoder.h)

### Interface: IVideoEncoder

**Purpose**: Abstract video encoder interface supporting H.264 hardware acceleration with low-latency optimization.

**Methods**:

```cpp
class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;

    // Lifecycle
    virtual bool initialize(const EncoderConfig& config) = 0;
    virtual void shutdown() = 0;

    // Encoding operation (non-blocking)
    virtual std::shared_ptr<EncodedFrame> encode(const VideoFrame& frame) = 0;

    // Quality control
    virtual void setTargetBitrate(int bps) = 0;
    virtual void setQPValue(int qp) = 0;
    virtual void requestKeyframe() = 0;

    // Capability queries
    virtual bool isHardwareAccelerated() const = 0;
    virtual std::vector<Resolution> getSupportedResolutions() const = 0;
    virtual int getMaxBitrate() const = 0;

    // Performance monitoring
    virtual float getAverageEncodingTimeMs() const = 0;
    virtual int getCurrentBitrate() const = 0;

    // Event callbacks
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};
```

**Data Structures**:

```cpp
struct EncoderConfig {
    int width;
    int height;
    int target_fps;
    int gop_size;  // 1 for I-frame every frame (lowest latency)
    int b_frames;  // 0 for no B-frames (low latency)
    int qp_value;  // 18-35 (lower = higher quality)
    int max_bitrate_bps;
    bool hardware_encode;
    std::string profile;  // "baseline", "main", "high"
};

struct EncodedFrame {
    std::vector<uint8_t> data;
    bool is_keyframe;
    uint64_t frame_id;
    int64_t timestamp_ns;
    int encoding_time_ms;
};
```

**Behavioral Guarantees**:
- `encode()` must complete within 10ms for hardware encoding (60fps target)
- `encode()` must complete within 20ms for software encoding fallback
- Memory allocation per encoded frame ≤ 2MB
- Hardware encoding must use NVENC (NVIDIA) or QuickSync (Intel)
- GOP size = 1 (I-frame every frame) to minimize latency
- No B-frames (B-frames increase decoding latency)

**Error Handling**:
- Throws `std::runtime_error` if encoder initialization fails
- Returns nullptr from `encode()` if input frame invalid
- Calls error callback if encoding fails repeatedly (>10 consecutive frames)
- Automatically falls back to software encoding if hardware unavailable (with warning)

---

## Contract 3: WebRTC Transport (webrtc_transport.h)

### Interface: IWebrtcTransport

**Purpose**: Abstract WebRTC transport layer for peer-to-peer video and data channel communication.

**Methods**:

```cpp
class IWebrtcTransport {
public:
    virtual ~IWebrtcTransport() = default;

    // Lifecycle
    virtual bool initialize(const TransportConfig& config) = 0;
    virtual void shutdown() = 0;

    // Connection management
    virtual std::string createOffer() = 0;
    virtual bool setRemoteDescription(const std::string& sdp) = 0;
    virtual bool addIceCandidate(const std::string& candidate) = 0;

    // Video track
    virtual void startVideoTrack(IVideoSource* source) = 0;
    virtual void stopVideoTrack() = 0;

    // Data channel
    virtual bool sendDataChannelMessage(const std::string& message) = 0;

    // State monitoring
    virtual ConnectionState getConnectionState() const = 0;
    virtual std::vector<IceCandidate> getLocalIceCandidates() const = 0;

    // Event callbacks
    virtual void setStateChangeCallback(std::function<void(ConnectionState)> callback) = 0;
    virtual void setIceCandidateCallback(std::function<void(const IceCandidate&)> callback) = 0;
    virtual void setDataChannelCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};
```

**Data Structures**:

```cpp
struct TransportConfig {
    std::string stun_server = "stun:stun.l.google.com:19302";
    bool use_ipv6 = false;
    int max_bitrate_bps = 15000000;
    bool enable_ice_tcp = false;
};

struct IceCandidate {
    std::string candidate;
    std::string sdp_mid;
    int sdp_mline_index;
};

enum class ConnectionState {
    NEW,
    CHECKING,
    CONNECTED,
    COMPLETED,
    FAILED,
    DISCONNECTED,
    CLOSED
};

class IVideoSource {
public:
    virtual ~IVideoSource() = default;
    virtual rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> getWebRTCSource() = 0;
};
```

**Behavioral Guarantees**:
- Connection establishment must complete within 5 seconds (including ICE negotiation)
- Video track must support 60fps @ 1080p minimum
- Data channel messages must be delivered in order and reliably
- Memory usage per connection ≤ 50MB
- Thread safety: Public methods can be called from any thread

**Error Handling**:
- Throws `std::runtime_error` if WebRTC initialization fails
- Calls error callback if ICE negotiation times out (30s)
- Automatically attempts reconnection if peer disconnects (configurable)
- Handles ICE candidate collection failures gracefully (log warning, continue)

---

## Contract 4: Input Processor (input_processor.h)

### Interface: IInputProcessor

**Purpose**: Abstract input processor for mapping mobile client input to Windows desktop actions.

**Methods**:

```cpp
class IInputProcessor {
public:
    virtual ~IInputProcessor() = default;

    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // Input processing (thread-safe, non-blocking)
    virtual void processMouseEvent(const MouseEvent& event) = 0;
    virtual void processKeyboardEvent(const KeyboardEvent& event) = 0;
    virtual void processGestureEvent(const GestureEvent& event) = 0;

    // Queue management
    virtual size_t getQueueSize() const = 0;
    virtual void clearQueue() = 0;

    // FIFO ordering
    virtual void processQueue() = 0;  // Blocking, processes all queued events

    // Performance monitoring
    virtual float getAverageProcessingTimeMs() const = 0;
    virtual size_t getTotalProcessedEvents() const = 0;

    // Event callbacks
    virtual void setErrorCallback(std::function<void(const std::string&, const InputEvent&)> callback) = 0;
};
```

**Data Structures**:

```cpp
struct MouseEvent {
    int x;
    int y;
    MouseButton button;
    MouseAction action;
    uint64_t timestamp_ns;
};

struct KeyboardEvent {
    int keycode;  // Windows virtual key code
    int modifiers;  // Bit flags: SHIFT=1, CTRL=2, ALT=4, WIN=8
    bool is_press;  // true for press, false for release
    uint64_t timestamp_ns;
};

struct GestureEvent {
    GestureType type;
    float scale;  // For PINCH_ZOOM
    int delta_x;  // For PAN
    int delta_y;  // For PAN
    uint64_t timestamp_ns;
};
```

**Behavioral Guarantees**:
- Input queue size ≤ 1000 events (configurable)
- Event processing must complete within 5ms per event (30ms budget - 25ms network/encoding)
- FIFO ordering enforced for all events (critical for multi-client coordination)
- Thread safety: `process*()` methods can be called from multiple threads
- Queue processing must be atomic (no partial state updates)

**Error Handling**:
- Discards events if queue full (log warning, continue)
- Calls error callback if SendInput API fails repeatedly (>5 consecutive failures)
- Automatically throttles event processing if average processing time > 10ms
- Validates event coordinates before processing (clamps to display bounds)

---

## Contract 5: Display Manager (display_manager.h)

### Interface: IDisplayManager

**Purpose**: Abstract display manager for multi-display enumeration, selection, and switching.

**Methods**:

```cpp
class IDisplayManager {
public:
    virtual ~IDisplayManager() = default;

    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // Display enumeration
    virtual std::vector<DisplaySource> enumerateDisplays() = 0;
    virtual DisplaySource getDisplayById(int id) = 0;
    virtual DisplaySource getPrimaryDisplay() = 0;

    // Display selection
    virtual bool selectDisplay(int display_id) = 0;
    virtual DisplaySource getSelectedDisplay() const = 0;
    virtual int getSelectedDisplayId() const = 0;

    // Display switching
    virtual bool switchDisplay(int display_id) = 0;  // <100ms requirement
    virtual void cancelPendingSwitch() = 0;

    // Display change monitoring
    virtual void setDisplayChangeCallback(std::function<void(const std::vector<DisplaySource>&)> callback) = 0;

    // Capability queries
    virtual bool supportsDisplayId(int display_id) const = 0;
    virtual int getMaxDisplayCount() const = 0;
};
```

**Behavioral Guarantees**:
- `switchDisplay()` must complete within 100ms (with brief frame interruption allowed)
- Display enumeration must complete within 500ms
- Must support up to 4 simultaneous displays
- Thread safety: Public methods can be called from any thread
- Display list refreshes automatically on configuration changes

**Error Handling**:
- Returns false if display_id invalid (0-3 range)
- Calls error callback if DXGI fails during display switch
- Recovers gracefully if display disconnected during session (fallback to next available)
- Logs warning if display list empty (no connected displays)

---

## Contract 6: Session Manager (session_manager.h) - Internal

### Interface: ISessionManager

**Purpose**: Manages multiple client sessions with FIFO input routing and reconnection handling.

**Methods**:

```cpp
class ISessionManager {
public:
    virtual ~ISessionManager() = default;

    // Lifecycle
    virtual bool initialize(const SessionManagerConfig& config) = 0;
    virtual void shutdown() = 0;

    // Session management
    virtual std::shared_ptr<Session> createSession(const std::string& client_ip) = 0;
    virtual bool removeSession(const std::string& session_id) = 0;
    virtual std::shared_ptr<Session> getSession(const std::string& session_id) = 0;
    virtual std::vector<std::shared_ptr<Session>> getAllSessions() const = 0;

    // Input routing (FIFO ordering)
    virtual bool routeInputEvent(const std::string& session_id, const InputEvent& event) = 0;
    virtual void processAllQueuedInputs() = 0;  // Single-threaded processing

    // Multi-display coordination
    virtual bool setSessionDisplay(const std::string& session_id, int display_id) = 0;
    virtual int getDisplaySubscriberCount(int display_id) const = 0;

    // Reconnection handling
    virtual bool handleReconnection(const std::string& session_id) = 0;
    virtual void startAutoReconnect(const std::string& session_id) = 0;
    virtual void stopAutoReconnect(const std::string& session_id) = 0;

    // Monitoring
    virtual MetricsSnapshot collectMetrics() const = 0;
    virtual int getActiveSessionCount() const = 0;
};
```

**Behavioral Guarantees**:
- Supports up to 4 simultaneous sessions (configurable)
- Input events processed in strict FIFO order across all clients
- Reconnection uses exponential backoff (1s, 2s, 4s, 8s... max 60s)
- Memory usage per session ≤ 200MB idle, 400MB peak
- Thread safety: Public methods can be called from WebRTC callbacks (multiple threads)

**Error Handling**:
- Returns nullptr from `createSession()` if max sessions reached (log warning)
- Silently discards input events for non-existent sessions (log error)
- Aborts reconnection after 10 failed attempts (manual intervention required)
- Calls error callback if session state inconsistency detected

---

## Performance Contract Summary

| Metric | Target | Monitoring Point |
|---------|----------|-------------------|
| Screen capture latency | ≤ 5ms | DXGICopyFrame timing |
| Encoding latency | ≤ 10ms (hardware), ≤ 20ms (software) | IVideoEncoder::encode() |
| Network transmission | ≤ 5ms (LAN) | WebRTC RTT measurement |
| End-to-end input latency | ≤ 30ms total | Client timestamp to screen response |
| Display switch latency | ≤ 100ms | IDisplayManager::switchDisplay() |
| Input queue processing | ≤ 5ms per event | IInputProcessor average |
| Session setup time | ≤ 5 seconds | Connection establish to screen visible |
| Reconnection time | ≤ 3 seconds (after network recovery) | ISessionManager reconnection |

---

## Threading Model

**WebRTC Callback Thread**: Handles ICE candidates, peer connection state changes, data channel messages

**Encoding Thread**: Dedicated thread for video encoding (one per session or shared thread pool)

**Network I/O Thread**: WebRTC networking operations (libwebrtc internal)

**Main Thread**: UI callbacks, session lifecycle management

**Worker Threads**: Input queue processing (single thread for FIFO ordering), background tasks (metrics collection, log rotation)

**Synchronization**: Minimal lock contention using atomic variables and lock-free queues where possible

---

## Error Codes

```cpp
enum class ErrorCode {
    SUCCESS = 0,

    // Capture errors
    DXGI_INIT_FAILED = 1000,
    DISPLAY_NOT_FOUND = 1001,
    CAPTURE_ALREADY_STARTED = 1002,
    FRAME_TIMEOUT = 1003,

    // Encoding errors
    ENCODER_INIT_FAILED = 2000,
    HARDWARE_ENCODER_UNAVAILABLE = 2001,
    ENCODING_FAILED = 2002,
    INVALID_RESOLUTION = 2003,

    // Transport errors
    WEBRTC_INIT_FAILED = 3000,
    ICE_NEGOTIATION_FAILED = 3001,
    PEER_CONNECTION_FAILED = 3002,
    DATA_CHANNEL_CLOSED = 3003,

    // Input errors
    INPUT_QUEUE_FULL = 4000,
    INVALID_INPUT_EVENT = 4001,
    COORDINATES_OUT_OF_BOUNDS = 4002,
    SENDINPUT_FAILED = 4003,

    // Session errors
    MAX_SESSIONS_REACHED = 5000,
    SESSION_NOT_FOUND = 5001,
    INVALID_SESSION_ID = 5002,
    RECONNECTION_ABORTED = 5003,

    // Display errors
    DISPLAY_SWITCH_FAILED = 6000,
    INVALID_DISPLAY_ID = 6001,
    DISPLAY_NOT_SUPPORTED = 6002,

    // System errors
    OUT_OF_MEMORY = 9000,
    THREAD_START_FAILED = 9001,
    CONFIGURATION_INVALID = 9002,
    UNKNOWN_ERROR = 9999
};
```
