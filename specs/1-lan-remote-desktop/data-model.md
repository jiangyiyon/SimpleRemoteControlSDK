# Data Model: LAN Low-Latency Remote Desktop SDK

**Feature**: 1-lan-remote-desktop
**Date**: 2025-02-11
**Status**: Draft

## Overview

This document defines the core data structures and entities used throughout the Remote Desktop SDK. All entities are designed for thread safety, low memory overhead, and efficient serialization for WebRTC transmission.

## Core Entities

### 1. Session

**Description**: Represents a remote desktop connection between Windows host and mobile client. Manages connection state, display selection, and lifecycle.

**Attributes**:

| Attribute | Type | Description | Constraints |
|-----------|--------|-------------|-------------|
| session_id | std::string | Unique session identifier (UUID v4) | 36 characters, uppercase hex |
| state | SessionState | Current connection state | ENUM: DISCONNECTED, CONNECTING, CONNECTED, RECONNECTING, ERROR |
| client_ip | std::string | Client IP address | IPv4 format, max 15 chars |
| connected_at | std::chrono::system_clock::time_point | Connection timestamp | UTC time |
| last_activity | std::chrono::system_clock::time_point | Last input/video frame timestamp | For timeout detection |
| display_source_id | int | Selected display index | 0-3 (max 4 displays) |
| latency_ms | std::atomic<int> | Current end-to-end latency | Updated every 100ms, 0-1000ms range |
| reconnection_attempts | std::atomic<int> | Number of reconnection attempts | Unlimited, exponential backoff |
| video_track_id | std::string | WebRTC video track identifier | Assigned by WebRTC peer connection |
| data_channel_id | std::string | WebRTC data channel ID | Assigned by WebRTC peer connection |

**Lifecycle**:
```
DISCONNECTED → CONNECTING → CONNECTED → [RECONNECTING*] → ERROR/RECONNECTED
                  ↓                                      ↓
                 CONNECTED ←----------------------------→ DISCONNECTED
```

**Thread Safety**: All atomic attributes (latency_ms, reconnection_attempts) can be accessed from multiple threads. Non-atomic attributes protected by mutex.

---

### 2. DisplaySource

**Description**: Represents a Windows display device that can be captured and transmitted.

**Attributes**:

| Attribute | Type | Description | Constraints |
|-----------|--------|-------------|-------------|
| id | int | Display index (enumeration order) | 0-3, unique per host |
| name | std::string | Display name (from DXGI) | Max 256 chars, UTF-8 |
| resolution_width | int | Horizontal resolution | 640-7680 (4K max) |
| resolution_height | int | Vertical resolution | 480-4320 (4K max) |
| refresh_rate | int | Refresh rate in Hz | 30-240 Hz |
| is_primary | bool | Primary display flag | Only one display can be true |
| is_active | std::atomic<bool> | Capture active state | Thread-safe |
| capture_handle | IDXGIOutputDuplication* | DXGI capture handle | Platform-specific, opaque |

**Methods**:
```cpp
bool supportsHardwareEncoding();  // Query NVENC/QuickSync capability
Resolution getNativeResolution();
void setCaptureResolution(const Resolution& res);  // Downscale for bandwidth
```

**Thread Safety**: is_active is atomic for concurrent capture/start/stop operations.

---

### 3. InputEvent

**Description**: Represents a user input action from mobile client to be executed on Windows desktop.

**Attributes**:

| Attribute | Type | Description | Constraints |
|-----------|--------|-------------|-------------|
| event_id | uint64_t | Monotonically increasing sequence number | FIFO ordering |
| event_type | InputEventType | Type of input event | ENUM: MOUSE_MOVE, MOUSE_CLICK, MOUSE_DRAG, KEYBOARD_PRESS, KEYBOARD_RELEASE, GESTURE |
| timestamp_ns | uint64_t | Event timestamp (nanoseconds) | For latency measurement |
| client_id | std::string | Originating client ID | Correlates with session_id |

**Event-Specific Payloads**:

**MouseEvent** (extends InputEvent):
| Attribute | Type | Description |
|-----------|--------|-------------|
| x | int | Screen X coordinate | 0 to display_width-1 |
| y | int | Screen Y coordinate | 0 to display_height-1 |
| button | MouseButton | LEFT, MIDDLE, RIGHT, NONE |
| action | MouseAction | PRESS, RELEASE, MOVE, DRAG |

**KeyboardEvent** (extends InputEvent):
| Attribute | Type | Description |
|-----------|--------|-------------|
| keycode | int | Windows virtual key code (VK_*) |
| modifiers | int | Modifier flags (SHIFT, CTRL, ALT, WIN) |

**GestureEvent** (extends InputEvent):
| Attribute | Type | Description |
|-----------|--------|-------------|
| gesture_type | GestureType | PINCH_ZOOM, PAN, LONG_PRESS, SWIPE |
| scale | float | Zoom factor (0.1x - 5.0x) |
| delta_x | int | Pan delta X (pixels) |
| delta_y | int | Pan delta Y (pixels) |

**Thread Safety**: Each InputEvent is immutable after creation, safe for concurrent read.

**Serialization** (for WebRTC data channel):
```json
{
  "event_id": 12345,
  "timestamp_ns": 17076604001234567,
  "type": "mouse_click",
  "payload": {
    "x": 1920,
    "y": 1080,
    "button": "left",
    "action": "press"
  }
}
```

---

### 4. VideoFrame

**Description**: Represents a captured and encoded screen frame ready for WebRTC transmission.

**Attributes**:

| Attribute | Type | Description | Constraints |
|-----------|--------|-------------|-------------|
| frame_id | uint64_t | Frame sequence number | Monotonically increasing |
| timestamp_ns | uint64_t | Capture timestamp | For synchronization |
| display_source_id | int | Source display ID | Correlates with DisplaySource |
| encoded_data | std::vector<uint8_t> | H.264 encoded data | 1-2MB typical size (1080p@60fps) |
| width | int | Frame width | 640-7680 |
| height | int | Frame height | 480-4320 |
| keyframe | bool | IDR (keyframe) flag | For decoder synchronization |
| encoding_time_ms | int | Encoding latency | 0-30ms, for monitoring |

**Memory Management**:
- Use shared_ptr for VideoFrame to enable zero-copy reference counting
- Implement object pool for VideoFrame reuse to reduce allocation overhead
- Enforce max 3 frames in flight per session (prevents memory buildup)

**Thread Safety**: All attributes immutable after encoding, safe for concurrent read.

---

### 5. ClientConnection

**Description**: Represents a connected mobile client with its WebRTC peer connection state.

**Attributes**:

| Attribute | Type | Description | Constraints |
|-----------|--------|-------------|-------------|
| connection_id | std::string | Unique connection ID | UUID v4 |
| ip_address | std::string | Client IP address | IPv4/IPv6 format |
| connection_state | PeerConnectionState | WebRTC peer state | ENUM: NEW, CONNECTING, CONNECTED, DISCONNECTED, FAILED, CLOSED |
| subscribed_display_id | int | Display source ID | 0-3, -1 for none |
| bandwidth_bps | std::atomic<int> | Current bitrate estimate | 500Kbps - 50Mbps |
| packet_loss_rate | std::atomic<float> | Packet loss percentage | 0.0 - 1.0 |
| rtt_ms | std::atomic<int> | Round-trip time | 0-500ms |
| connected_at | std::chrono::system_clock::time_point | Connection timestamp | UTC time |
| last_heartbeat | std::chrono::system_clock::time_point | Last activity timestamp | For connection health monitoring |

**Methods**:
```cpp
void updateBandwidthEstimate(int bps);  // From WebRTC stats
void updateLatencyMetrics(int rtt_ms, float packet_loss);
bool isHealthy();  // RTT < 300ms && packet_loss < 5%
void sendDataChannelMessage(const std::string& message);  // Thread-safe
```

**Thread Safety**: All numeric metrics (bandwidth_bps, packet_loss_rate, rtt_ms) are atomic for concurrent WebRTC callback updates.

---

## Data Structures

### Configuration

**Description**: SDK configuration loaded from JSON file (config/default.json).

**Structure**:

```cpp
struct Configuration {
    // Server settings
    std::string bind_address = "0.0.0.0";
    int port = 8080;

    // Video settings
    int target_fps = 60;
    int max_resolution_width = 1920;
    int max_resolution_height = 1080;
    std::string default_display = "0";
    bool hardware_encoding = true;

    // Encoding settings (low-latency optimization)
    int gop_size = 1;  // I-frame every frame
    int b_frames = 0;  // No B-frames for latency
    int qp_value = 23;  // Quality parameter (18-35)
    int max_bitrate_bps = 15000000;  // 15Mbps max

    // Performance settings
    int max_sessions = 4;
    int input_queue_size = 1000;
    int frame_queue_size = 3;  // Max frames in flight

    // Logging settings
    std::string log_level = "INFO";  // TRACE, DEBUG, INFO, WARN, ERROR
    std::string log_file = "logs/screensdk.log";
    bool log_to_console = false;
    int log_max_size_mb = 100;
    int log_max_files = 5;

    // Reconnection settings
    bool auto_reconnect = true;
    int reconnect_max_delay_ms = 60000;  // 60s max backoff
    float reconnect_backoff_multiplier = 2.0f;
};
```

**Thread Safety**: Configuration is read-only after SDK initialization, no synchronization needed.

---

### MetricsSnapshot

**Description**: Periodic performance metrics collected for monitoring and debugging.

**Structure**:

```cpp
struct MetricsSnapshot {
    // Capture metrics
    int capture_fps;
    float avg_capture_time_ms;
    int dropped_frames;

    // Encoding metrics
    int encoding_fps;
    float avg_encoding_time_ms;
    int current_bitrate_bps;

    // Network metrics (per client)
    std::map<std::string, ClientMetrics> client_metrics;

    // System metrics
    float cpu_usage_percent;
    size_t memory_usage_mb;
    int active_sessions;

    // Quality metrics
    float avg_latency_ms;
    float packet_loss_percent;
};
```

**ClientMetrics** (nested):
```cpp
struct ClientMetrics {
    std::string client_id;
    int rtt_ms;
    float packet_loss_rate;
    int bandwidth_bps;
    int frames_sent;
    int frames_dropped;
    std::chrono::system_clock::time_point last_activity;
};
```

---

## Enumerations

```cpp
enum class SessionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    ERROR
};

enum class InputEventType {
    MOUSE_MOVE,
    MOUSE_CLICK,
    MOUSE_DRAG,
    KEYBOARD_PRESS,
    KEYBOARD_RELEASE,
    GESTURE
};

enum class MouseButton {
    LEFT,
    MIDDLE,
    RIGHT,
    NONE
};

enum class MouseAction {
    PRESS,
    RELEASE,
    MOVE,
    DRAG
};

enum class GestureType {
    PINCH_ZOOM,
    PAN,
    LONG_PRESS,
    SWIPE
};

enum class PeerConnectionState {
    NEW,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    FAILED,
    CLOSED
};
```

---

## Thread Safety Model

**Read-Only Data**: Configuration, static enumerations - No synchronization required

**Atomic Variables**: Session::latency_ms, Session::reconnection_attempts, DisplaySource::is_active, ClientConnection::metrics - Use std::atomic<T>

**Mutable Shared Data**: Session collection, input queue - Protect with std::mutex or use lock-free data structures (boost::lockfree or custom ring buffer)

**Event Loop**: Single-threaded WebRTC signaling/main loop, worker thread pool for encoding and I/O

**Design Pattern**: Producer-consumer for video frames and input events to minimize lock contention

---

## Memory Management

**RAII Wrappers**: All WebRTC resources (PeerConnection, VideoTrack) wrapped in RAII classes

**Smart Pointers**: std::shared_ptr for VideoFrame, std::unique_ptr for owned resources

**Object Pooling**: VideoFrame pool (size = max_sessions * frame_queue_size) to reduce allocation overhead

**Memory Limits**: Enforced max memory per session (200MB idle, 400MB peak) with monitoring alerts

---

## Validation Rules

**Session Validation**:
- session_id must be valid UUID v4
- display_source_id must be 0-3 and correspond to existing DisplaySource
- latency_ms must be 0-1000ms

**InputEvent Validation**:
- x, y must be within display resolution bounds
- keycode must be valid Windows virtual key code (0-255)
- event_id must be monotonically increasing per client

**VideoFrame Validation**:
- encoded_data size must be 1-4MB (prevents memory explosion)
- width/height must match display resolution
- keyframe flag must be true for frame_id = 0 or every 250 frames (10s at 60fps)

**Configuration Validation**:
- port must be 1024-65535
- target_fps must be 30-120
- max_sessions must be 1-10
- hardware_encoding requires GPU detection validation
