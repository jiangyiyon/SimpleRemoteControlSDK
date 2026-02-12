# Architecture Design: LAN Low-Latency Remote Desktop SDK

**Document Version**: 2.0 (UML 2.0 Standard)  
**Date**: 2025-02-11  
**Feature**: 1-lan-remote-desktop  
**Status**: Draft  
**UML Version**: 2.0

## Table of Contents

1. [Technical Solution Overview](#technical-solution-overview)
2. [System Architecture Diagrams](#system-architecture-diagrams)
3. [Layered Architecture](#layered-architecture)
4. [Module Design](#module-design)
5. [Interface Diagrams](#interface-diagrams)
6. [Sequence Diagrams](#sequence-diagrams)
7. [Activity Diagrams](#activity-diagrams)
8. [Component Diagrams](#component-diagrams)
9. [Deployment Diagrams](#deployment-diagrams)
10. [Package Diagrams](#package-diagrams)

---

## Technical Solution Overview

### 1.1 Technical Architecture

The Remote Desktop SDK implements a **Clean Architecture** pattern with five layers, ensuring separation of concerns and testability.

#### Key Technologies

| Component | Technology | Purpose |
|------------|------------|---------|
| Server Language | C++20 | High-performance native implementation |
| Client Language | JavaScript ES2020 | Chrome browser compatibility |
| Video Transport | WebRTC (libwebrtc) | Low-latency real-time streaming |
| Screen Capture | DXGI Desktop Duplication API | Hardware-accelerated capture |
| Video Encoding | H.264 (NVENC/QuickSync/x264) | Efficient video compression |
| Signaling | WebSocket | SDP exchange and ICE negotiation |
| Build System | CMake | Cross-platform build configuration |
| Testing | Google Test (C++), Jest (JS) | Comprehensive test coverage |

#### Performance Targets

| Metric | Target | Method |
|--------|--------|--------|
| End-to-end latency | ≤30ms | Optimized pipeline, lock-free queues |
| Video frame rate | 60fps | Hardware encoding, minimal buffering |
| CPU usage | <30% (idle) | Hardware acceleration |
| Memory usage | <200MB (idle) | Object pooling, shared_ptr |
| Network bandwidth | 5-15Mbps @1080p@60fps | Adaptive bitrate |
| Display switch time | <100ms | Hot-switch, minimal disruption |
| Max concurrent clients | 4 | Tested and validated |

---

## System Architecture Diagrams

### 2.1 High-Level Architecture Diagram

```plantuml
@startuml HighLevelArchitecture
!define RECTANGLE class
!define LAYER rect

skinparam backgroundColor #FEFEFE
skinparam activityBorderColor #000000
skinparam activityBackgroundColor #E0E0E0

title High-Level Architecture - Clean Architecture

package "Layer 5: Platform Layer" {
    rectangle "WindowsInputAPI\n<<platform>>" as wininput
    rectangle "DisplayEnumerator\n<<platform>>" as displayenum
    rectangle "GPUDetector\n<<platform>>" as gpudetector
}

package "Layer 4: Transport Layer" {
    rectangle "DXGIScreenCapture\n<<adapter>>" as dxgi
    rectangle "NVENCEncoder\n<<adapter>>" as nvenc
    rectangle "QSVSEncoder\n<<adapter>>" as qsv
    rectangle "SoftwareEncoder\n<<adapter>>" as swenc
    rectangle "WebRTCManager\n<<adapter>>" as webrtc
}

package "Layer 3: Business Logic Layer" {
    rectangle "SessionManager" as sessionmgr
    rectangle "InputRouter" as inputrouter
    rectangle "DisplayController" as displayctrl
    rectangle "ReconnectionHandler" as reconnect
}

package "Layer 2: API Layer" {
    rectangle "IScreenSDK\n<<interface>>" as isdk
    rectangle "ISession\n<<interface>>" as isession
    rectangle "ISDKCallback\n<<interface>>" as isdkcb
    rectangle "ISessionCallback\n<<interface>>" as isessioncb
}

package "Layer 1: Presentation Layer" {
    rectangle "VideoRenderer" as vidrend
    rectangle "InputCapture" as inputcap
    rectangle "GestureRecognizer" as gesture
    rectangle "DisplaySelector" as displaysel
}

' Dependencies
webrtc ..> isdk : uses
sessionmgr ..> isession : creates
sessionmgr ..> dxgi : uses
sessionmgr ..> nvenc : uses
sessionmgr ..> webrtc : uses
inputrouter ..> wininput : uses
displayctrl ..> displayenum : uses
dxgi ..> displayenum : uses
nvenc ..> gpudetector : uses
isdk ..> isdkcb : notifies
isession ..> isessioncb : notifies

@enduml
```

### 2.2 System Context Diagram

```plantuml
@startuml SystemContext
!define RECTANGLE class
skinparam backgroundColor #FEFEFE

title System Context Diagram

actor "Mobile User" as user
actor "Windows User" as host

rectangle "Remote Desktop SDK" {
    rectangle "Chrome Browser\n(Presentation)" as client
    rectangle "Windows Server\n(Business Logic)" as server
    
    client ..> server : WebRTC
    server ..> client : H.264 Video Stream
    client ..> server : Input Commands
}

rectangle "External Systems" {
    database "Network\n(LAN)" as network
    rectangle "Windows OS\n(Platform)" as os
}

client -- network : HTTPS/WebSocket
server -- network : UDP/TCP
server -- os : DXGI, SendInput, etc.
user -- client : Touch/Keyboard
host -- server : Desktop Display

@enduml
```

---

## Layered Architecture

### 3.1 Layer Dependency Diagram

```plantuml
@startuml LayerDependency
!define RECTANGLE class
skinparam layerarrowColor #000000

title Layer Dependency Diagram

layer Presentation {
    rectangle "VideoRenderer" as V1
    rectangle "InputCapture" as V2
    rectangle "GestureRecognizer" as V3
}

layer API {
    rectangle "IScreenSDK" as A1
    rectangle "ISession" as A2
    rectangle "Configuration" as A3
}

layer BusinessLogic {
    rectangle "SessionManager" as B1
    rectangle "InputRouter" as B2
    rectangle "DisplayController" as B3
}

layer Transport {
    rectangle "IScreenCapture" as T1
    rectangle "IVideoEncoder" as T2
    rectangle "IWebRTCManager" as T3
}

layer Platform {
    rectangle "DXGIScreenCapture" as P1
    rectangle "NVENCEncoder" as P2
    rectangle "WebRTCManager" as P3
}

V1 ..> A1 : depends on
V2 ..> A2 : depends on
A1 ..> B1 : depends on
A2 ..> B2 : depends on
B1 ..> T1 : depends on
B1 ..> T2 : depends on
B2 ..> T3 : depends on
T1 ..> P1 : implements
T2 ..> P2 : implements
T3 ..> P3 : implements

legend right
  Dependency direction follows Clean Architecture rule
  (outer layer depends on inner layer)
endlegend

@enduml
```

### 3.2 Package Structure Diagram

```plantuml
@startuml PackageStructure
skinparam packageStyle rectangle

title Package Structure

package screensdk {
    
    package "api" {
        class IScreenSDK <<interface>>
        class ISession <<interface>>
        class ISDKCallback <<interface>>
        class ISessionCallback <<interface>>
        class Configuration
        class Result<T>
    }
    
    package "core" {
        class SessionManager
        class Session
        class InputRouter
        class DisplayController
        class ReconnectionHandler
    }
    
    package "capture" {
        class IScreenCapture <<interface>>
        class DXGIScreenCapture
        class CapturePipeline
        class VideoFrame
    }
    
    package "encoding" {
        class IVideoEncoder <<interface>>
        class EncoderFactory
        class NVENCEncoder
        class QSVSEncoder
        class SoftwareEncoder
        class EncoderConfig
    }
    
    package "transport" {
        class IWebRTCManager <<interface>>
        class WebRTCManager
        class IVideoSource <<interface>>
        class CustomVideoTrackSource
        class DataChannel
        class SignalingServer
    }
    
    package "input" {
        class IInputProcessor <<interface>>
        class InputProcessor
        class MouseHandler
        class KeyboardHandler
        class InputMapper
        class GestureHandler
    }
}

package "platform" {
    class WindowsInputAPI
    class DisplayEnumerator
    class GPUDetector
}

' Internal dependencies
SessionManager ..> IScreenCapture
SessionManager ..> IVideoEncoder
SessionManager ..> IWebRTCManager
InputRouter ..> IInputProcessor

@enduml
```

---

## Module Design

### 4.1 API Module Class Diagram

```plantuml
@startuml APIModule
skinparam classAttributeIconSize 0

title API Module Class Diagram

interface IScreenSDK {
    + {abstract} initialize(config: Configuration): Result<void>
    + {abstract} start(bind_address: string, port: int): Result<string>
    + {abstract} stop(): void
    + {abstract} createSession(): Result<ISession>
    + {abstract} getActiveSessions(): vector<ISession>
    + {abstract} updateConfig(config: Configuration): Result<void>
    + {abstract} getConfig(): Configuration
    + {abstract} registerCallback(callback: ISDKCallback): void
    + {abstract} getMetrics(): MetricsSnapshot
}

interface ISession {
    + {abstract} connect(client_id: string): Result<void>
    + {abstract} disconnect(): void
    + {abstract} isConnected(): bool
    + {abstract} getAvailableDisplays(): vector<DisplayInfo>
    + {abstract} selectDisplay(display_id: int): Result<void>
    + {abstract} getCurrentDisplay(): DisplayInfo
    + {abstract} sendInput(event: InputEvent): void
    + {abstract} getSessionId(): string
    + {abstract} getState(): SessionState
    + {abstract} getLatency(): int
    + {abstract} registerCallback(callback: ISessionCallback): void
}

interface ISDKCallback {
    + {abstract} onSessionCreated(session_id: string): void
    + {abstract} onSessionTerminated(session_id: string, reason: string): void
    + {abstract} onError(error: Error): void
}

interface ISessionCallback {
    + {abstract} onConnected(info: SessionInfo): void
    + {abstract} onDisconnected(reason: string): void
    + {abstract} onError(error: Error): void
    + {abstract} onLatencyUpdate(latency_ms: int): void
    + {abstract} onDisplaySwitched(display: DisplayInfo): void
}

class Configuration {
    - bind_address: string
    - port: int
    - target_fps: int
    - max_resolution_width: int
    - max_resolution_height: int
    - hardware_encoding: bool
    - max_sessions: int
    + Configuration()
    + loadFromFile(path: string): Result<Configuration>
    + validate(): Result<void>
}

class Result<T> {
    <<template>>
    + success(value: T): Result<T>
    + error(message: string): Result<T>
    + isSuccess(): bool
    + isError(): bool
    + getValue(): T
    + getError(): string
}

note right of IScreenSDK
  Main SDK entry point
  Factory function: CreateScreenSDK()
  Cleanup function: DestroyScreenSDK()
end note

IScreenSDK --> ISession : creates 1..*
IScreenSDK --> ISDKCallback : uses 1
ISession --> ISessionCallback : uses 1
IScreenSDK --> Configuration : uses
ISession --> InputEvent : uses

@enduml
```

### 4.2 Business Logic Module Class Diagram

```plantuml
@startuml BusinessLogicModule
skinparam classAttributeIconSize 0

title Business Logic Module Class Diagram

class SessionManager {
    - session_map_: map<string, Session>
    - input_router_: InputRouter
    - display_controller_: DisplayController
    - reconnection_handler_: ReconnectionHandler
    - session_mutex_: mutex
    + createSession(client_id: string): Result<Session>
    + getSession(session_id: string): Session*
    + terminateSession(session_id: string): Result<void>
    + broadcastFrame(frame: VideoFrame): void
    + routeInput(event: InputEvent): void
    + getActiveSessions(): vector<Session>
}

class Session {
    - session_id_: string
    - state_: SessionState
    - client_ip_: string
    - display_source_id_: int
    - latency_ms_: atomic<int>
    - webrtc_connection_: IWebRTCManager
    - callback_: ISessionCallback
    + connect(client_id: string): Result<void>
    + disconnect(): void
    + sendInput(event: InputEvent): void
    + selectDisplay(display_id: int): Result<void>
    + getState(): SessionState
    + getLatency(): int
}

class InputRouter {
    - event_queue_: LockFreeQueue<InputEvent>
    - processing_thread_: thread
    - active_: atomic<bool>
    + queueInput(client_id: string, event: InputEvent): void
    + processBatch(): vector<InputEvent>
    + stop(): void
}

class DisplayController {
    - display_map_: map<int, DisplaySource>
    - current_display_id_: int
    - display_mutex_: mutex
    + enumerateDisplays(): vector<DisplayInfo>
    + selectDisplay(display_id: int): Result<void>
    + getCurrentDisplay(): DisplayInfo
    + detectChanges(): vector<DisplayChange>
}

class ReconnectionHandler {
    - retry_count_: int
    - backoff_ms_: int
    - max_backoff_ms_: int
    - retry_timer_: Timer
    + startReconnect(session_id: string): void
    + stopReconnect(): void
    + handleSuccess(): void
    + handleFailure(): void
}

enum SessionState {
    DISCONNECTED
    CONNECTING
    CONNECTED
    RECONNECTING
    ERROR
}

SessionManager "1" *-- "*" Session : manages
SessionManager "1" *-- "1" InputRouter : owns
SessionManager "1" *-- "1" DisplayController : owns
SessionManager "1" *-- "1" ReconnectionHandler : owns
Session --> ISessionCallback : notifies

@enduml
```

### 4.3 Screen Capture Module Class Diagram

```plantuml
@startuml ScreenCaptureModule
skinparam classAttributeIconSize 0

title Screen Capture Module Class Diagram

interface IScreenCapture {
    + {abstract} initialize(display_id: int): Result<void>
    + {abstract} start(): Result<void>
    + {abstract} stop(): void
    + {abstract} captureFrame(): Result<VideoFrame>
    + {abstract} enumerateDisplays(): vector<DisplayInfo>
}

class DXGIScreenCapture {
    - duplication_: IDXGIOutputDuplication*
    - device_: ID3D11Device*
    - context_: ID3D11DeviceContext*
    - display_id_: int
    - frame_pool_: ObjectPool<VideoFrame>
    + initialize(display_id: int): Result<void>
    + start(): Result<void>
    + stop(): void
    + captureFrame(): Result<VideoFrame>
    + enumerateDisplays(): vector<DisplayInfo>
}

class CapturePipeline {
    - capturer_: IScreenCapture*
    - converter_: FormatConverter
    - frame_queue_: LockFreeQueue<VideoFrame>
    - capture_thread_: thread
    - active_: atomic<bool>
    + start(): void
    + stop(): void
    + getFrame(): optional<VideoFrame>
    + onFrame(callback: function): void
}

class VideoFrame {
    - frame_id_: uint64_t
    - timestamp_ns_: uint64_t
    - display_source_id_: int
    - encoded_data_: vector<uint8_t>
    - width_: int
    - height_: int
    - keyframe_: bool
    - encoding_time_ms_: int
    + getFrameId(): uint64_t
    + getTimestamp(): uint64_t
    + getData(): vector<uint8_t>&
    + isKeyframe(): bool
}

class DisplayInfo {
    + id: int
    + name: string
    + width: int
    + height: int
    + refresh_rate: int
    + is_primary: bool
}

IScreenCapture <|.. DXGIScreenCapture : implements
CapturePipeline "1" --> "1" IScreenCapture : uses
CapturePipeline "1" *-- "1" VideoFrame : produces

note right of DXGIScreenCapture
  DXGI Desktop Duplication API
  Hardware-accelerated capture
  < 2ms latency
end note

@enduml
```

### 4.4 Video Encoding Module Class Diagram

```plantuml
@startuml VideoEncodingModule
skinparam classAttributeIconSize 0

title Video Encoding Module Class Diagram

interface IVideoEncoder {
    + {abstract} initialize(config: EncoderConfig): Result<void>
    + {abstract} encode(frame: VideoFrame, callback: function): Result<void>
    + {abstract} release(): void
    + {abstract} supportsHardwareEncoding(): bool
}

class NVENCEncoder {
    - nvenc_session_: NvEncSession*
    - encoder_: NvEncoder*
    - input_buffer_: ID3D11Texture2D*
    + initialize(config: EncoderConfig): Result<void>
    + encode(frame: VideoFrame, callback: function): Result<void>
    + release(): void
    + supportsHardwareEncoding(): bool
}

class QSVSEncoder {
    - qsv_session_: MFXSession*
    - encoder_: MFXVideoENCODE*
    - allocator_: MFXAllocator*
    + initialize(config: EncoderConfig): Result<void>
    + encode(frame: VideoFrame, callback: function): Result<void>
    + release(): void
    + supportsHardwareEncoding(): bool
}

class SoftwareEncoder {
    - x264_context_: x264_t*
    - encoder_params_: x264_param_t
    + initialize(config: EncoderConfig): Result<void>
    + encode(frame: VideoFrame, callback: function): Result<void>
    + release(): void
    + supportsHardwareEncoding(): bool
}

class EncoderFactory {
    - gpu_detector_: GPUDetector
    + create(config: EncoderConfig): unique_ptr<IVideoEncoder>
    + createHardware(config: EncoderConfig): unique_ptr<IVideoEncoder>
    + createSoftware(config: EncoderConfig): unique_ptr<IVideoEncoder>
    + detectBestEncoder(): EncoderType
}

class EncoderConfig {
    + codec: string
    + width: int
    + height: int
    + fps: int
    + bitrate: int
    + gop_size: int
    + qp_value: int
    + b_frames: int
    + hardware_enabled: bool
}

class EncodedFrame {
    + frame_id: uint64_t
    + timestamp_ns: uint64_t
    + keyframe: bool
    + data: vector<uint8_t>
    + size_bytes: int
    + encoding_time_ms: int
}

IVideoEncoder <|.. NVENCEncoder : implements
IVideoEncoder <|.. QSVSEncoder : implements
IVideoEncoder <|.. SoftwareEncoder : implements
EncoderFactory ..> IVideoEncoder : creates
EncoderFactory ..> GPUDetector : uses

note right of EncoderFactory
  Factory Pattern
  Auto-detect GPU capabilities
  Graceful degradation to software
end note

@enduml
```

### 4.5 WebRTC Transport Module Class Diagram

```plantuml
@startuml WebRTCTransportModule
skinparam classAttributeIconSize 0

title WebRTC Transport Module Class Diagram

interface IWebRTCManager {
    + {abstract} startServer(address: string, port: int): Result<void>
    + {abstract} stopServer(): void
    + {abstract} createPeerConnection(client_id: string): Result<string>
    + {abstract} closePeerConnection(connection_id: string): void
    + {abstract} sendVideoTrack(connection_id: string, source: IVideoSource): Result<void>
    + {abstract} sendDataChannelMessage(connection_id: string, message: string): Result<void>
    + {abstract} registerCallback(callback: IWebRTCCallback): void
}

class WebRTCManager {
    - peer_factory_: PeerConnectionFactory*
    - connections_: map<string, PeerConnection>
    - signaling_server_: SignalingServer
    - callback_: IWebRTCCallback*
    + startServer(address: string, port: int): Result<void>
    + stopServer(): void
    + createPeerConnection(client_id: string): Result<string>
    + closePeerConnection(connection_id: string): void
    + sendVideoTrack(connection_id: string, source: IVideoSource): Result<void>
    + sendDataChannelMessage(connection_id: string, message: string): Result<void>
}

interface IVideoSource {
    + {abstract} start(): void
    + {abstract} stop(): void
    + {abstract} onFrame(callback: function): void
    + {abstract} getFrame(): VideoFrame
}

class CustomVideoTrackSource {
    - frame_queue_: LockFreeQueue<VideoFrame>
    - frame_width_: int
    - frame_height_: int
    + start(): void
    + stop(): void
    + pushFrame(frame: VideoFrame): void
    + onFrame(callback: function): void
}

class DataChannel {
    - channel_: RTCDataChannel*
    - buffer_: vector<InputEvent>
    + send(message: string): Result<void>
    + onMessage(callback: function): void
    + isOpen(): bool
}

class SignalingServer {
    - ws_server_: WebSocketServer
    - port_: int
    + start(port: int): void
    + stop(): void
    + handleOffer(client_id: string, sdp: string): void
    + handleAnswer(client_id: string, sdp: string): void
    + handleIceCandidate(client_id: string, candidate: string): void
}

interface IWebRTCCallback {
    + {abstract} onConnected(connection_id: string): void
    + {abstract} onDisconnected(connection_id: string): void
    + {abstract} onError(error: Error): void
    + {abstract} onIceCandidate(candidate: string): void
}

IWebRTCManager <|.. WebRTCManager : implements
IVideoSource <|.. CustomVideoTrackSource : implements
WebRTCManager "1" --> "*" DataChannel : manages
WebRTCManager "1" --> "1" SignalingServer : uses
WebRTCManager --> IWebRTCCallback : notifies

@enduml
```

### 4.6 Input Processing Module Class Diagram

```plantuml
@startuml InputProcessingModule
skinparam classAttributeIconSize 0

title Input Processing Module Class Diagram

interface IInputProcessor {
    + {abstract} processInput(event: InputEvent): Result<void>
    + {abstract} registerGestureCallback(callback: IGestureCallback): void
}

class InputProcessor {
    - mouse_handler_: MouseHandler
    - keyboard_handler_: KeyboardHandler
    - gesture_handler_: GestureHandler
    - input_mapper_: InputMapper
    + processInput(event: InputEvent): Result<void>
    + registerGestureCallback(callback: IGestureCallback): void
}

class MouseHandler {
    - input_api_: WindowsInputAPI*
    + sendMouseMove(x: int, y: int): void
    + sendMouseClick(button: MouseButton, action: MouseAction): void
    + sendMouseDrag(x: int, y: int): void
}

class KeyboardHandler {
    - input_api_: WindowsInputAPI*
    + sendKeyPress(keycode: uint8_t): void
    + sendKeyRelease(keycode: uint8_t): void
    + sendKeyCombo(keys: vector<uint8_t>): void
}

class InputMapper {
    - client_width_: int
    - client_height_: int
    - desktop_width_: int
    - desktop_height_: int
    + touchToMouse(touch: TouchEvent): MouseEvent
    + touchToKeyboard(touch: TouchEvent): KeyboardEvent
    + normalizeCoords(x: int, y: int): pair<int, int>
    + gestureToInput(gesture: GestureEvent): InputEvent
}

class GestureHandler {
    - touch_tracker_: TouchTracker
    - gesture_thresholds_: GestureThresholds
    + handleTouch(touches: vector<Touch>): GestureEvent
    + handlePinch(scale: float): GestureEvent
    + handlePan(delta_x: int, delta_y: int): GestureEvent
    + handleLongPress(duration: int): GestureEvent
    + handleSwipe(direction: SwipeDirection): GestureEvent
}

class WindowsInputAPI {
    - input_stream_: INPUT*
    + sendMouseMove(x: int, y: int): void
    + sendMouseClick(button: MouseButton, action: MouseAction): void
    + sendKeyboardInput(keycode: uint8_t, pressed: bool): void
    + sendInput(input: INPUT): void
}

class InputEvent {
    + event_id: uint64_t
    + event_type: InputEventType
    + timestamp_ns: uint64_t
    + client_id: string
    + payload: Payload
}

class GestureEvent {
    + gesture_type: GestureType
    + scale: float
    + delta_x: int
    + delta_y: int
    + duration_ms: int
}

IInputProcessor <|.. InputProcessor : implements
InputProcessor "1" *-- "1" MouseHandler : owns
InputProcessor "1" *-- "1" KeyboardHandler : owns
InputProcessor "1" *-- "1" GestureHandler : owns
InputProcessor "1" *-- "1" InputMapper : owns
MouseHandler ..> WindowsInputAPI : uses
KeyboardHandler ..> WindowsInputAPI : uses

@enduml
```

---

## Interface Diagrams

### 5.1 Public Interface Overview

```plantuml
@startuml PublicInterfaceOverview
skinparam packageStyle rectangle

title Public Interface Overview

interface IScreenSDK {
    initialize()
    start()
    stop()
    createSession()
    getActiveSessions()
    updateConfig()
    getConfig()
    registerCallback()
    getMetrics()
}

interface ISession {
    connect()
    disconnect()
    isConnected()
    getAvailableDisplays()
    selectDisplay()
    getCurrentDisplay()
    sendInput()
    getSessionId()
    getState()
    getLatency()
    registerCallback()
}

interface ISDKCallback {
    onSessionCreated()
    onSessionTerminated()
    onError()
}

interface ISessionCallback {
    onConnected()
    onDisconnected()
    onError()
    onLatencyUpdate()
    onDisplaySwitched()
}

' Relationships
IScreenSDK --> ISession : creates
IScreenSDK --> ISDKCallback : notifies
ISession --> ISessionCallback : notifies

note right of IScreenSDK
  Factory: CreateScreenSDK()
  Destroy: DestroyScreenSDK(sdk)
  
  Usage:
  auto sdk = CreateScreenSDK();
  sdk->initialize(config);
  sdk->start("0.0.0.0", 8080);
end note

@enduml
```

### 5.2 Transport Layer Interfaces

```plantuml
@startuml TransportInterfaces
skinparam packageStyle rectangle

title Transport Layer Interfaces

interface IScreenCapture {
    initialize(display_id)
    start()
    stop()
    captureFrame()
    enumerateDisplays()
}

interface IVideoEncoder {
    initialize(config)
    encode(frame, callback)
    release()
    supportsHardwareEncoding()
}

interface IWebRTCManager {
    startServer(address, port)
    stopServer()
    createPeerConnection(client_id)
    closePeerConnection(id)
    sendVideoTrack(id, source)
    sendDataChannelMessage(id, msg)
    registerCallback()
}

interface IVideoSource {
    start()
    stop()
    onFrame(callback)
    getFrame()
}

' Dependencies
IScreenCapture <|.. DXGIScreenCapture
IVideoEncoder <|.. NVENCEncoder
IVideoEncoder <|.. QSVSEncoder
IWebRTCManager <|.. WebRTCManager
IVideoSource <|.. CustomVideoTrackSource

@enduml
```

---

## Sequence Diagrams

### 6.1 Session Establishment Sequence Diagram

```plantuml
@startuml SessionEstablishment
skinparam backgroundColor #FEFEFE

title Session Establishment Sequence Diagram

actor "Mobile User" as user
participant "VideoRenderer" as renderer
participant "ISession" as session
participant "SessionManager" as smgr
participant "WebRTCManager" as webrtc
participant "SignalingServer" as signaling
participant "CustomVideoTrackSource" as source

user -> renderer: Connect to Server (IP + Port)
renderer -> session: connect(client_id)
session -> smgr: Register session
smgr -> session: Create session object
session -> webrtc: createPeerConnection(client_id)
webrtc --> session: Return connection_id
webrtc -> signaling: Send SDP Offer
signaling -> webrtc: Receive SDP Answer
webrtc -> source: Start video track
source --> webrtc: Supply frames
webrtc -> signaling: Send ICE candidates
signaling -> webrtc: Receive ICE candidates
webrtc -> webrtc: Establish PeerConnection
session -> session: setState(CONNECTED)
session -> renderer: onConnected(session_info)
renderer --> user: Display desktop
note right: Connection established\nwithin 5 seconds

@enduml
```

### 6.2 Video Capture and Transmission Sequence Diagram

```plantuml
@startuml VideoCaptureTransmission
skinparam backgroundColor #FEFEFE

title Video Capture and Transmission Sequence Diagram

participant "DisplayController" as dc
participant "DXGIScreenCapture" as capture
participant "NVENCEncoder" as encoder
participant "CapturePipeline" as pipeline
participant "CustomVideoTrackSource" as source
participant "WebRTCManager" as webrtc
participant "Client Browser" as client

dc -> capture: start()
capture -> capture: AcquireNextFrame()
capture --> capture: IDXGISurface
capture -> pipeline: queueFrame(raw_frame)
pipeline -> encoder: encode(frame, callback)
encoder -> encoder: NvEncEncodePicture()
encoder --> pipeline: EncodedFrame(data)
pipeline -> source: pushFrame(encoded_frame)
source -> webrtc: SendVideoTrack(frame)
webrtc -> webrtc: RTP Packetization
webrtc -> client: Send RTP packets
client -> client: Hardware H.264 decode
client -> client: Render to Canvas/WebGL

note over capture, client
  Total Latency: 16-30ms
  - Capture: 1-2ms
  - Encoding: 5-10ms
  - Transmission: 5-10ms
  - Decoding: 5-8ms
end note

@enduml
```

### 6.3 Input Event Processing Sequence Diagram

```plantuml
@startuml InputEventProcessing
skinparam backgroundColor #FEFEFE

title Input Event Processing Sequence Diagram

actor "Mobile User" as user
participant "InputCapture" as inputcap
participant "GestureRecognizer" as gesture
participant "InputMapper" as mapper
participant "DataChannel" as dc
participant "WebRTCManager" as webrtc
participant "InputRouter" as router
participant "MouseHandler" as mouse
participant "Windows OS" as os

user -> inputcap: Touch screen (x, y)
inputcap -> gesture: handleTouch(touches)
gesture -> gesture: Recognize gesture type
gesture -> mapper: gestureToInput(gesture)
mapper -> mapper: Normalize coordinates
mapper -> dc: send(json_input)
dc -> webrtc: sendDataChannelMessage()
webrtc -> router: queueInput(client_id, event)
router -> router: Process FIFO queue
router -> mouse: sendMouseEvent(x, y, action)
mouse -> os: SendInput(INPUT_MOUSE)
os -> os: Move cursor on desktop
note right of os: Input executed\nwithin 25-32ms

note over inputcap, os
  FIFO ordering guaranteed
  for multi-client scenarios
end note

@enduml
```

### 6.4 Display Switching Sequence Diagram

```plantuml
@startuml DisplaySwitching
skinparam backgroundColor #FEFEFE

title Display Switching Sequence Diagram

actor "Mobile User" as user
participant "DisplaySelector" as selector
participant "ISession" as session
participant "DisplayController" as dc
participant "DXGIScreenCapture" as capture
participant "CapturePipeline" as pipeline
participant "VideoRenderer" as renderer

user -> selector: Select Display 1 from menu
selector -> session: selectDisplay(display_id=1)
session -> dc: selectDisplay(display_id=1)
dc -> capture: stop()
capture --> dc: Frame release complete
dc -> capture: initialize(display_id=1)
capture --> dc: Capture ready
dc -> pipeline: Switch source
pipeline -> renderer: onDisplaySwitched(display_info)
renderer -> renderer: Update canvas size
renderer --> user: Show Display 1

note over selector, renderer
  Display switch < 100ms
  Brief frame interruption allowed
  State preserved across switch
end note

@enduml
```

### 6.5 Reconnection Sequence Diagram

```plantuml
@startuml Reconnection
skinparam backgroundColor #FEFEFE

title Reconnection Sequence Diagram

participant "Session" as session
participant "ReconnectionHandler" as rh
participant "WebRTCManager" as webrtc
participant "Network" as net
participant "Signaling" as sig

net -> session: Connection lost
session -> rh: startReconnect()
rh -> rh: Initialize backoff (100ms)
rh -> webrtc: Attempt reconnect
webrtc -> sig: Send SDP Offer
net -> net: Network timeout
webrtc --> rh: Connection failed
rh -> rh: Exponential backoff (200ms)
rh -> webrtc: Retry reconnect
webrtc -> sig: Send SDP Offer
sig --> webrtc: SDP Answer received
webrtc --> session: onConnected()
rh -> rh: handleSuccess()
rh -> rh: Reset retry count
session --> session: Restore session state
note right: Reconnected within 3 seconds

note over rh
  Backoff progression:
  100ms → 200ms → 400ms → ...
  Max: 60 seconds
  Indefinite retry
end note

@enduml
```

---

## Activity Diagrams

### 7.1 SDK Initialization Activity Diagram

```plantuml
@startuml SDKInitialization
skinparam backgroundColor #FEFEFE

title SDK Initialization Activity Diagram

start

:Create IScreenSDK\nvia CreateScreenSDK();
:Load Configuration\nfrom config/default.json;
:Validate Configuration;

if (Valid?) then (No)
  :Log error;
  stop
else (Yes)
  :Initialize SessionManager;
  :Initialize DisplayController;
  :Initialize InputRouter;
  :Initialize ReconnectionHandler;
  :Detect GPU Capabilities\n(NVENC, QuickSync);
  
  if (Hardware Encoding?) then (Yes)
    :Create NVENCEncoder\nor QSVSEncoder;
  else (No)
    :Create SoftwareEncoder;
    :Log warning;
  endif
  
  :Initialize WebRTCManager;
  :Start Signaling Server\non port 8080;
  
  partition "Start Capture" {
    :Enumerate Displays;
    :Start Capture on Display 0;
    :Start Capture Pipeline;
  }
  
  :SDK Ready;
  stop
endif

@enduml
```

### 7.2 Video Frame Processing Activity Diagram

```plantuml
@startuml VideoFrameProcessing
skinparam backgroundColor #FEFEFE

title Video Frame Processing Activity Diagram

start

:Duplication Thread:\nAcquire DXGI Frame;

if (Frame available?) then (No)
  :Wait for frame (16ms @60fps);
  stop
endif

:Convert Surface\nto NV12 format;
:Allocate VideoFrame\nfrom object pool;

:Duplication Thread:\nQueue to CapturePipeline;

:Encoder Thread:\nDequeue from Queue;

:Hardware Encode\n(NVENC/QuickSync);

if (Encoding success?) then (No)
  :Log error;
  :Retry with software encoder;
  stop
endif

:Enqueue EncodedFrame;

:WebRTC Thread:\nRead EncodedFrame;

:RTP Packetization;

:Send to Client via UDP;

:Client Decode\n(Hardware H.264);

:Render to Canvas/WebGL;

:Release VideoFrame\nto object pool;

stop

note right
  Pipeline optimized for
  zero-copy passing and
  minimal latency
end note

@enduml
```

### 7.3 Input Event Handling Activity Diagram

```plantuml
@startuml InputEventHandling
skinparam backgroundColor #FEFEFE

title Input Event Handling Activity Diagram

start

:Client Touch Event;

partition "Client Processing" {
  :Capture touch coordinates;
  :Detect gesture type\n(pinch, pan, tap, etc.);
  :Normalize coordinates\n(client → desktop);
  :Serialize to JSON;
}

partition "Transmission" {
  :Send via WebRTC DataChannel;
  :Ordered delivery (SCTP);
}

partition "Server Processing" {
  :Receive and parse JSON;
  :Validate input bounds;
  :Queue to InputRouter;
}

:FIFO Processing\n(single-threaded);

:Dispatch to Handler;

if (Gesture?) then (Yes)
  :Client-side only\n(no Windows input);
  stop
endif

partition "Windows Input" {
  if (Mouse?) then (Yes)
    :SendInput(INPUT_MOUSE);
    :Move/click cursor;
  elseif (Keyboard?) then (Yes)
    :SendInput(INPUT_KEYBOARD);
    :Send key press/release;
  endif
}

:Desktop responds;

:Next frame captures\ninput result;

stop

@enduml
```

### 7.4 Multi-Client Session Management Activity Diagram

```plantuml
@startuml MultiClientSessionManagement
skinparam backgroundColor #FEFEFE

title Multi-Client Session Management Activity Diagram

start

:Session Manager Started;

while (New client request?) is (Yes)
  :Create Session object;
  :Generate unique session_id;
  :Initialize WebRTC PeerConnection;
  :Subscribe to current display;
  
  if (Max sessions reached?) then (Yes = 4)
    :Reject connection;
    stop
  endif
  
  :Add to session_map_;
  :Notify callback\nonSessionCreated();
  :Wait for WebRTC handshake;
  
  if (Connection successful?) then (Yes)
    :Set session state CONNECTED;
    :Start video stream;
  else (No)
    :Cleanup failed session;
    :Notify callback onError();
  endif
  
  :Client connected;
endwhile (No)

while (Client disconnects?) is (Yes)
  :Remove from session_map_;
  :Close PeerConnection;
  :Cleanup resources;
  :Notify callback\nonSessionTerminated();
endwhile (No)

stop

note right
  Max 4 concurrent clients
  FIFO input ordering
  Shared display source
end note

@enduml
```

---

## Component Diagrams

### 8.1 Server Component Diagram

```plantuml
@startuml ServerComponent
skinparam componentStyle uml2

title Server Component Diagram

component "IScreenSDK" as sdk {
    port "initialize()" as p1
    port "start()" as p2
    port "createSession()" as p3
}

component "SessionManager" as sm {
    port "routeInput()" as p4
    port "broadcastFrame()" as p5
}

component "DisplayController" as dc {
    port "selectDisplay()" as p6
    port "enumerateDisplays()" as p7
}

component "InputRouter" as ir {
    port "processBatch()" as p8
}

component "CapturePipeline" as cp {
    port "getFrame()" as p9
}

component "WebRTCManager" as wm {
    port "sendVideoTrack()" as p10
    port "sendDataChannel()" as p11
}

component "SignalingServer" as ss {
    port "handleOffer()" as p12
    port "handleAnswer()" as p13
}

component "DXGIScreenCapture" as dxc
component "NVENCEncoder" as nvenc
component "WindowsInputAPI" as winapi

' Connectors
sdk ..> sm : creates
sdk ..> dc : creates
sdk ..> ir : creates
sm ..> cp : uses
sm ..> wm : uses
sm ..> winapi : uses
ir ..> winapi : uses
cp ..> dxc : uses
cp ..> nvenc : uses
wm ..> ss : manages
wm --> wm : WebRTC
wm --> wm : H.264 Stream
wm --> wm : Input Commands

@enduml
```

### 8.2 Client Component Diagram

```plantuml
@startuml ClientComponent
skinparam componentStyle uml2

title Client Component Diagram

component "RemoteDesktopClient" as client {
    port "connect()" as c1
    port "disconnect()" as c2
}

component "VideoRenderer" as vr {
    port "onFrame()" as v1
    port "onDisplaySwitched()" as v2
}

component "InputCapture" as ic {
    port "onTouch()" as i1
    port "onKeyPress()" as i2
}

component "GestureRecognizer" as gr {
    port "recognize()" as g1
}

component "ViewController" as vc {
    port "zoom()" as vc1
    port "pan()" as vc2
}

component "DisplaySelector" as ds {
    port "onSelect()" as d1
}

component "MetricsDisplay" as md {
    port "updateMetrics()" as m1
}

component "WebRTCConnection" as wc {
    port "sendVideoOffer()" as w1
    port "sendInputMessage()" as w2
}

' Connectors
client ..> vr : creates
client ..> ic : creates
client ..> wc : creates
client ..> gr : uses
client ..> vc : uses
client ..> ds : uses
client ..> md : uses
ic ..> gr : delegates
wc ..> vr : delivers frames
wc ..> ds : delivers display list
vr --> wc : subscribe video
ic --> wc : send input

@enduml
```

---

## Deployment Diagrams

### 9.1 Network Deployment Diagram

```plantuml
@startuml NetworkDeployment
skinparam componentStyle uml2

title Network Deployment Diagram

node "Windows Host\n(192.168.1.100)" as server {
    component "screensdk_server.exe" as svr
    component "screensdk.dll" as dll
    component "Configuration\nconfig.json" as cfg
    component "Logs\nlogs/" as logs
    artifact "DXGI API" as dxgi
    artifact "NVENC SDK" as nvenc
}

node "Router" as router
node "Switch" as sw

node "Mobile Client 1\n(192.168.1.101)" as client1 {
    component "Chrome Browser" as ch1
}

node "Mobile Client 2\n(192.168.1.102)" as client2 {
    component "Chrome Browser" as ch2
}

node "Mobile Client 3\n(192.168.1.103)" as client3 {
    component "Chrome Browser" as ch3
}

' Connections
svr -- router : TCP 8080 (Signaling)
svr -- router : UDP 50000-59999 (Media)
client1 -- router : Wi-Fi/4G
client2 -- router : Wi-Fi/4G
client3 -- router : Wi-Fi/4G
router -- sw : LAN
sw -- router : LAN

svr --> dll : loads
svr --> cfg : reads
svr --> logs : writes
svr -dxgi : uses
svr -nvenc : uses

ch1 -[hidden]down- svr : WebRTC
ch2 -[hidden]down- svr : WebRTC
ch3 -[hidden]down- svr : WebRTC

@enduml
```

### 9.2 Hardware Deployment Diagram

```plantuml
@startuml HardwareDeployment
skinparam componentStyle uml2

title Hardware Deployment Diagram

node "Windows PC" as pc {
    artifact "CPU\nIntel i5/AMD Ryzen 5" as cpu
    artifact "GPU\nNVIDIA GTX 1650\nor Intel Arc A380" as gpu
    artifact "RAM\n8GB" as ram
    artifact "Network\nGigabit LAN" as net
    
    component "Remote Desktop SDK" as sdk
    component "WebRTC Library" as webrtc
    component "GPU Drivers" as drivers
}

node "Displays" as displays {
    node "Display 0\n1920x1080@60Hz" as disp0
    node "Display 1\n1920x1080@60Hz" as disp1
}

node "Mobile Devices" as mobile {
    device "Phone 1\nChrome Mobile" as phone1
    device "Tablet 1\nChrome Mobile" as tablet1
}

' Connections
sdk -cpu : runs on
sdk -gpu : hardware encode
sdk -ram : uses
sdk -net : binds to
sdk -webrtc : integrates
sdk -drivers : DX11, NVENC
disp0 -gpu : connected to
disp1 -gpu : connected to
mobile -net : Wi-Fi connection

note right of gpu
  Required for
  H.264 hardware encoding
  (5-10ms latency)
  
  Fallback: Software encoding
  (15-20ms latency)
end note

@enduml
```

---

## Package Diagrams

### 10.1 Complete Package Dependency Diagram

```plantuml
@startuml PackageDependency
skinparam packageStyle rectangle

title Package Dependency Diagram

package "screensdk" {
    
    package "api" as api {
        interface IScreenSDK
        interface ISession
        interface ISDKCallback
        interface ISessionCallback
        class Configuration
        class Result<T>
    }
    
    package "core" as core {
        class SessionManager
        class Session
        class InputRouter
        class DisplayController
        class ReconnectionHandler
    }
    
    package "capture" as cap {
        interface IScreenCapture
        class DXGIScreenCapture
        class CapturePipeline
        class VideoFrame
    }
    
    package "encoding" as enc {
        interface IVideoEncoder
        class EncoderFactory
        class NVENCEncoder
        class QSVSEncoder
        class SoftwareEncoder
    }
    
    package "transport" as trans {
        interface IWebRTCManager
        interface IVideoSource
        class WebRTCManager
        class CustomVideoTrackSource
        class DataChannel
    }
    
    package "input" as inp {
        interface IInputProcessor
        class InputProcessor
        class MouseHandler
        class KeyboardHandler
        class GestureHandler
    }
}

package "platform" as plat {
    class WindowsInputAPI
    class DisplayEnumerator
    class GPUDetector
}

package "presentation" as pres {
    class VideoRenderer
    class InputCapture
    class GestureRecognizer
    class DisplaySelector
}

' Dependencies
pres ..> api : depends
api ..> core : depends
core ..> cap : depends
core ..> enc : depends
core ..> trans : depends
core ..> inp : depends
cap ..> plat : depends
enc ..> plat : depends
trans ..> plat : depends
inp ..> plat : depends

note right
  Dependency Rule:
  Outer layers depend on inner layers
  Inner layers have no dependencies
  on outer layers
end note

@enduml
```

### 10.2 Module Interaction Diagram

```plantuml
@startuml ModuleInteraction
skinparam packageStyle rectangle

title Module Interaction Diagram

rectangle "API Layer" {
    component "IScreenSDK" as isdk
    component "ISession" as isession
}

rectangle "Business Logic" {
    component "SessionManager" as sm
    component "InputRouter" as ir
    component "DisplayController" as dc
}

rectangle "Transport" {
    component "CapturePipeline" as cp
    component "WebRTCManager" as wm
}

rectangle "Platform" {
    component "DXGIScreenCapture" as dxc
    component "WindowsInputAPI" as wia
}

' Interactions
isdk -> sm : createSession()
isession -> sm : connect()
sm -> dc : selectDisplay()
sm -> ir : routeInput()
ir -> wia : sendMouseMove()
sm -> wm : createPeerConnection()
wm -> sm : onConnected()
sm -> cp : getFrame()
cp -> dxc : captureFrame()

note right of sm
  Central orchestrator
  Coordinates all modules
  Maintains session state
end note

@enduml
```

---

## Appendix: UML 2.0 Notation Reference

### Notation Legend

| Symbol | Meaning |
|---------|---------|
| `interface` | Interface/Abstract class |
| `class` | Concrete class |
| `<<interface>>` | Stereotype for interface |
| `<<factory>>` | Factory pattern |
| `<<adapter>>` | Adapter pattern |
| `<<template>>` | Generic/template class |
| `-->` | Association |
| `..>` | Dependency |
| `*--` | Composition |
| `o--` | Aggregation |
| `<|--` | Inheritance/Implementation |
| `+` | Public member |
| `-` | Private member |
| `#` | Protected member |

### Relationship Types

```
Inheritance:        Subclass <|-- Superclass
Implementation:     Class <|.. Interface
Association:        ClassA --> ClassB
Dependency:         ClassA ..> ClassB
Composition:        Container *-- Component
Aggregation:        Container o-- Component
Realization:        Interface <|.. Implementation
```

---

**End of Document**
