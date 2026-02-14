# Feature Specification: LAN Low-Latency Remote Desktop SDK

**Feature Branch**: `1-lan-remote-desktop`
**Created**: 2025-02-11
**Status**: Draft
**Input**: User description: "局域网低延迟远程桌面SDK"

## User Scenarios & Testing

### TDD Workflow (NON-NEGOTIABLE - Per Constitution)

**All user stories MUST follow this test-first approach:**

1. **Test Specification**: Define acceptance scenarios with Given-When-Then format
2. **User Approval**: Stakeholder reviews and approves test scenarios BEFORE implementation
3. **Test Implementation**: Write automated tests that FAIL (Red phase)
4. **Feature Implementation**: Implement code to make tests PASS (Green phase)
5. **Refactoring**: Improve code quality while maintaining passing tests (Refactor phase)

**Mandatory Quality Gates:**

- ✅ Test cases written and approved by user
- ✅ All tests FAIL before implementation begins
- ✅ All tests PASS after implementation
- ✅ Code coverage ≥ 80% for core logic
- ✅ No regressions in existing tests

---

### User Story 1 - Basic Remote Desktop Connection (Priority: P1)

A mobile device user wants to view and control their Windows desktop from their mobile device using the Chrome browser within the same local network. The user can see the desktop screen in real-time and perform basic input operations (mouse movements, clicks, keyboard input) with minimal delay.

**Why this priority**: This is the core value proposition of the SDK - enabling remote desktop access with real-time control. Without this, there is no usable product.

**Independent Test**: Can be fully tested by establishing a connection between a Windows host and a mobile Chrome browser on a local network with <20ms RTT, then performing input operations and verifying the desktop responds within the defined latency threshold.

**Acceptance Scenarios**:

1. **Given** Windows server is running on local network, **When** mobile user enters server IP address and connects via Chrome browser, **Then** connection is established within 5 seconds and the desktop screen is visible
2. **Given** active remote desktop connection, **When** user performs mouse movement on mobile screen, **Then** cursor on Windows desktop moves correspondingly within 30ms (one-way: action → display)
3. **Given** active remote desktop connection, **When** user taps on mobile screen to click, **Then** Windows desktop registers the click at correct location within 30ms (one-way: action → display)
4. **Given** active remote desktop connection, **When** user types characters using virtual keyboard, **Then** characters appear on Windows desktop within 30ms (one-way: action → display)

---

### User Story 2 - Multi-Display Switching (Priority: P2)

A Windows user with multiple connected displays wants to view and control a specific display from their mobile device. The user can select which display to view and switch between displays during an active remote session.

**Why this priority**: Many Windows users use multiple monitors. The ability to select and switch between displays significantly enhances usability and makes the SDK suitable for more use cases.

**Independent Test**: Can be tested on a Windows system with 2+ connected displays by verifying the mobile client can enumerate, select, and switch between different displays with minimal interruption to the session.

**Acceptance Scenarios**:

1. **Given** Windows host with multiple displays, **When** mobile user opens display selection menu, **Then** list of available displays shows correct names and resolutions
2. **Given** active remote connection, **When** user selects a different display from the menu, **Then** mobile view switches to selected display within 100ms
3. **Given** active remote connection to Display 1, **When** user switches to Display 2 and performs input, **Then** input is correctly routed to Display 2
4. **Given** Windows display configuration changes (display added/removed/resolution changed), **When** user refreshes display list, **Then** updated display list reflects current configuration

---

### User Story 3 - Touch Gesture Support (Priority: P3)

A mobile user wants to use natural touch gestures on their mobile device to control the remote Windows desktop. The user can perform pinch-to-zoom, pan, and other touch gestures to navigate the desktop and interact with applications.

**Why this priority**: Touch gestures are the primary interaction method on mobile devices. Providing intuitive gesture support enhances user experience and makes the remote control feel more natural on mobile devices.

**Independent Test**: Can be tested by performing various touch gestures on a mobile device and verifying the corresponding actions occur correctly on the remote Windows desktop.

**Acceptance Scenarios**:

1. **Given** active remote connection, **When** user performs pinch-to-zoom gesture on mobile screen, **Then** remote desktop view zooms in/out accordingly (client-side view transformation only)
2. **Given** zoomed remote desktop view, **When** user performs pan gesture (drag with two fingers), **Then** mobile client view canvas scrolls to show different areas of the desktop (client-side only)
3. **Given** active remote connection, **When** user performs long-press gesture, **Then** right-click action is triggered on Windows desktop
4. **Given** active remote connection, **When** user performs swipe gesture, **Then** appropriate scroll action occurs on Windows desktop

---

### User Story 4 - Multi-Client Support (Priority: P4)

Multiple users want to connect to the same Windows desktop simultaneously from their mobile devices for collaborative viewing and control (e.g., troubleshooting, presentations, training).

**Why this priority**: Supporting multiple concurrent connections extends the SDK's use cases to collaborative scenarios, making it more valuable for team environments.

**Independent Test**: Can be tested by connecting 3 different mobile devices to the same Windows host simultaneously and verifying all devices receive the screen stream and can send input.

**Acceptance Scenarios**:

1. **Given** Windows server running, **When** first client connects, **Then** connection succeeds and desktop is visible
2. **Given** one active client connection, **When** second and third clients connect, **Then** both new connections succeed and all three clients see the desktop
3. **Given** 3 active client connections, **When** any client sends input, **Then** Windows desktop responds to input from that client
4. **Given** 3 active client connections, **When** one client disconnects, **Then** remaining clients continue to function normally

---

### Edge Cases

- What happens when network connection is interrupted during an active session? System should attempt automatic reconnection with exponential backoff (initial: 1s, maximum: 30s, multiplier: 2x)
- What happens when Windows host display configuration changes (display added/removed)? System should detect change via DXGI, pause video stream (not data channel), notify user via data channel message, allow display list refresh, and restore stream when user selects valid display
- What happens when Windows host GPU does not support hardware encoding? System should fall back to software encoding or display error with error type enum and details string
- What happens when mobile device Chrome browser does not support required WebRTC features? System should detect incompatibility and display error with error type enum and details string
- What happens when network latency exceeds 30ms threshold? System should maintain connection and display latency warning when latency exceeds 100ms for >5 consecutive seconds (threshold: display warning when >100ms for >5s)
- What happens when multiple clients send conflicting input simultaneously? System should process inputs in order received
- What happens when Windows host resolution changes during active session? System should adapt to new resolution without crashing
- What happens when client device screen orientation changes? Remote view should adjust to maintain proper aspect ratio

## Requirements

### Functional Requirements

- **FR-001**: System MUST capture screen content from Windows desktop at 60±5 frames per second (resolution-independent requirement applies to all supported resolutions: 720p, 1080p, 1440p, 4K)
- **FR-002**: System MUST transmit captured screen content to mobile client using WebRTC protocol
- **FR-003**: System MUST encode video stream using H.264 codec with hardware acceleration when available, and fall back to software encoding with performance warning when hardware encoding is unavailable
- **FR-004**: System MUST decode video stream on mobile client using browser native H.264 decoder
- **FR-005**: System MUST transmit input commands from mobile client to Windows host with one-way latency of 30ms or less (action → display)
- **FR-006**: System MUST support mouse movement, click, and drag operations from mobile client
- **FR-007**: System MUST support keyboard input from mobile client virtual keyboard, including keyboard modifier keys (Ctrl, Alt, Shift)
- **FR-008**: System MUST enumerate all connected displays on Windows host
- **FR-009**: System MUST allow mobile client to select and view specific display
- **FR-010**: System MUST support switching between displays within 100ms during active session, with brief frame interruption allowed during transition
- **FR-011**: System supports up to 4 simultaneous client connections (recommended maximum for optimal performance)
- **FR-012**: System MUST support pinch-to-zoom and pan gestures on mobile client for view manipulation only (does not affect actual Windows desktop)
- **FR-013**: System MUST map mobile touch events to corresponding Windows input events
- **FR-014**: System MUST display real-time latency metrics to user
- **FR-015**: System MUST attempt automatic reconnection when network interruption is detected, retrying indefinitely with exponential backoff (initial: 1s, maximum: 30s, multiplier: 2x)
- **FR-016**: System MUST provide bandwidth estimation and packet loss tracking per client connection

### Key Entities

**TDD Requirement**: For all entities below, unit tests MUST be written and approved by user BEFORE implementation begins. Tests must FAIL initially (Red phase), implementation follows (Green phase).

- **Connection Session**: Represents an active remote desktop connection between Windows host and mobile client, containing state (DISCONNECTED, CONNECTING, CONNECTED, RECONNECTING, ERROR), display selection, and input routing information
- **Display Source**: Represents a Windows display device that can be captured, containing display name, resolution, and capture pipeline information
- **Input Event**: Represents a user input action from mobile client, containing event type (mouse, keyboard, gesture), coordinates, and payload data
- **Video Frame**: Represents a captured screen frame ready for transmission, containing encoded H.264 data, timestamp (frame capture time in milliseconds since session start), and display source identifier
- **Client Connection**: Represents a connected mobile client, containing IP address, WebRTC peer connection state, and subscribed display source
- **Error Type**: Enumeration of error categories (NETWORK_ERROR, ENCODING_ERROR, DECODING_ERROR, INPUT_ERROR, CAPTURE_ERROR, BROWSER_INCOMPATIBILITY, HARDWARE_UNAVAILABLE), used for error handling and user-facing error messages
- **Error Details**: Structured error information containing error type enum, error code, timestamp, and human-readable message string for debugging and user feedback

## Success Criteria

### Measurable Outcomes

- **SC-001**: Users can establish remote desktop connection within 5 seconds of entering server IP address
- **SC-002**: One-way input latency (action → display on mobile) is 30ms or less in local network environment
- **SC-003**: System maintains 60±5 frames per second video stream at any supported resolution (720p, 1080p, 1440p, 4K) (minimum acceptable: 55fps for 99% of time under normal network conditions)
- **SC-004**: Display switch operation completes within 100ms from user action to new display visible on mobile
- **SC-005**: System can support 3 simultaneous client connections without performance degradation (performance degradation defined as >20% increase in latency or >10% decrease in frame rate)
- **SC-006**: 90% of input actions result in correct response on Windows desktop within latency threshold
- **SC-007**: System operates continuously for 24 hours without crashes or memory leaks
- **SC-008**: Users can successfully navigate and control Windows desktop applications using touch gestures on first attempt
- **SC-009**: Automatic reconnection succeeds within 3 seconds after network interruption recovery
- **SC-010**: System adapts to display configuration changes without requiring server restart

- **SC-011**: When multiple clients send conflicting input simultaneously, system processes inputs using FIFO (first-in-first-out) policy based on arrival order

## Clarifications

### Latency Measurement Methodology (U1)

**Definition**: End-to-end one-way latency measures the time from user action (T1) to visual feedback display (T8).

**Measurement Points**:
```
┌─────────────┐                    ┌──────────────┐
│ Mobile      │                    │ Windows Host │
│ Client      │                    │              │
└──────┬──────┘                    └──────┬───────┘
       │                                  │
   T1: User action (touch/click)          │
       │                                  │
   T2: Send over WebRTC data channel      │
       │                                  │
   T3: Network transmission ──────────────►│
       │                              T4: Windows processes input
       │                                  │
       ◄───────── Frame capture ──────────┤T5
       │                                  │
   T6: Network transmission (video)       │
       │                                  │
   T7: Render frame on mobile client      │
       │                                  │
   T8: Visual feedback on screen         │
       │                                  │
```

**End-to-End Latency = T8 - T1**

| Point | Description | Precision |
|-------|-------------|-----------|
| T1 | User action timestamp (touch/click event on mobile client) | ±1ms (performance.now()) |
| T2 | Input transmission timestamp (WebRTC data channel send) | ±1ms |
| T3 | Network transmission (client → host) | Implicit |
| T4 | Input processing timestamp (Windows host receives input) | ±1ms |
| T5 | Windows response (after SendInput() API call) | Implicit |
| T6 | Frame capture timestamp (video frame encoding) | ±1ms |
| T7 | Network transmission (host → client) | Implicit |
| T8 | Display timestamp (frame rendered on mobile client) | ±1ms (requestAnimationFrame) |

**Calculation**:
- **One-way latency** = T8 - T1 (end-to-end)
- **Network latency** = (T4 - T2 + T7 - T6) / 2 (average RTT/2)
- **Capture+encode latency** = T6 - T5
- **Decode+render latency** = T8 - T7

**Verification Method**:
1. Mobile client captures T1 (touch event) and includes in input message
2. Windows host receives input at T4, processes, captures frame at T6
3. Mobile client receives frame, renders at T8
4. Client calculates end-to-end latency = T8 - T1
5. Client displays latency metrics and logs for analysis

**Test Scenarios**:
- **Baseline test**: 100 consecutive clicks, measure average, P50, P95, P99
- **Stress test**: Rapid clicks (10 clicks/second), verify no latency degradation
- **Network test**: Simulate 10ms, 20ms, 50ms RTT, verify latency scales linearly
- **Multi-client test**: 4 clients simultaneously, verify no latency degradation

**Acceptance Criteria**:
- Average one-way latency ≤ 30ms (P50)
- 95th percentile latency ≤ 40ms (P95)
- 99th percentile latency ≤ 50ms (P99)
- Maximum latency spike < 100ms (except network interruption)

### Display Switching Protocol Specification (U4)

**Purpose**: Define the WebRTC SDP renegotiation protocol behavior for seamless display switching during active remote sessions. Display switching is achieved through WebRTC media renegotiation, which causes a brief, controlled frame interruption.

**Protocol Overview**:

Display switching uses WebRTC Session Description Protocol (SDP) renegotiation to replace the video track source without breaking the ICE connection. This approach balances speed (≤100ms requirement) with stability (maintains established network path).

**Switching Process Flow**:

```
┌─────────────┐                    ┌──────────────┐
│ Mobile      │                    │ Windows Host │
│ Client      │                    │              │
└──────┬──────┘                    └──────┬───────┘
       │                                  │
  1. User selects new display              │
       │                                  │
  2. Send "switchDisplay" message          │
     (via WebRTC data channel) ──────────►│
       │                                  │
       │                           3. Receive request
       │                           4. Stop current encoder
       │                           5. Switch DxgiCapture to
       │                              new display source
       │                           6. Start encoder with
       │                              new display
       │                                  │
       │ ◄───────── 7. Create new SDP offer│
       │    (re-negotiation request)       │
       │                                  │
  8. Process SDP offer                     │
     (ICE connection REUSED)               │
       │                                  │
  9. Create SDP answer ───────────────────►│
       │                          10. Process answer
       │                          11. Replace video track
       │                          12. Resume frame transmission
       │                                  │
 13. Receive new video track               │
 14. Switch decoder to new track          │
 15. Display new video frame              │
       │                                  │
  [Frame interruption: ~5-10 frames at 60fps ≈ 83-166ms]
       │                                  │
```

**Frame Interruption Behavior**:

During display switching, temporary frame loss occurs due to:

1. **Encoder Stop/Start** (~10-20ms): Encoder must be stopped and restarted with new display source
2. **SDP Renegotiation** (~30-50ms): WebRTC exchanges offer/answer without ICE restart
3. **Decoder Switch** (~10-20ms): Client switches decoder to new video track
4. **Buffer Replenishment** (~10-20ms): Video decoder buffer refills with new frames

**Total Interruption**: ~60-110ms (typically ~80ms at 60fps)

**Frame Loss Estimate**: ~5-7 frames at 60fps during transition

**Key Protocol Details**:

**ICE Connection Behavior**:
- ✅ **ICE connection is NOT restarted** - established network path is preserved
- ✅ Only media (video track) is renegotiated
- ✅ Data channel remains active during renegotiation
- ⚠️ Temporary video track replacement causes frame gap

**SDP Renegotiation Sequence**:

```
Step 1: Host initiates renegotiation
  - Sends: RTCPeerConnection.setLocalDescription(offer)
  - Offer includes: New video track with new display source

Step 2: Client receives offer
  - Calls: RTCPeerConnection.setRemoteDescription(offer)
  - Creates answer with new track acceptance

Step 3: Host receives answer
  - Calls: RTCPeerConnection.setRemoteDescription(answer)
  - Switches video source to new display

Step 4: New video stream begins
  - Client receives first frame from new display
  - Decoder processes new track
  - Video updates on client display
```

**Timing Breakdown**:

| Step | Component | Duration | Notes |
|------|-----------|----------|-------|
| 1-2 | Request transmission | ~5ms | Data channel, very fast |
| 3-6 | Display switch on host | ~15ms | DXGI reconfiguration |
| 7 | SDP offer creation | ~10ms | WebRTC signaling |
| 8-9 | SDP offer/answer exchange | ~30ms | Network + processing |
| 10-12 | Host side track replacement | ~20ms | WebRTC pipeline |
| 13-15 | Client side track switch | ~20ms | Decoder switch |
| **Total** | **End-to-end** | **~100ms** | **Meets ≤100ms requirement** |

**Frame Loss Specification**:

- **Maximum acceptable frame loss**: ≤7 frames at 60fps (~116ms)
- **Typical frame loss**: 5-6 frames at 60fps (~83-100ms)
- **Expected interruption duration**: 60-110ms
- **Stream pause behavior**: ❌ Stream does NOT pause - brief gap in video frames

**Error Handling**:

**Host-side Errors**:
- Display switch failure (display not found/disabled):
  - Send error response via data channel
  - Keep existing display stream active
  - User can retry or select different display

**Client-side Errors**:
- SDP renegotiation failure:
  - Display error message to user
  - Attempt to re-establish connection to current display
  - Allow user to manually reconnection

**Network Errors**:
- Renegotiation timeout (>200ms):
  - Abort renegotiation
  - Restore previous display stream
  - Notify user and allow retry

**Testing Requirements**:

| Test Scenario | Expected Behavior |
|---------------|-------------------|
| Switch between 2 displays | Complete within 100ms, 5-7 frame loss |
| Switch to same display (no-op) | Should be detected and skipped (no renegotiation) |
| Switch during high CPU load | Still within 100ms, may increase to 6-8 frame loss |
| Switch to invalid display | Error returned, existing stream maintained |
| Switch during network latency (20ms) | Still within 100ms, frame loss unchanged |
| Rapid consecutive switches (3 in 1 second) | Each switch completes within 100ms, no degradation |

**Implementation Notes**:

- Use WebRTC `replaceTrack()` API when available for faster switching
- Prefer `track.onunmute` event to detect when new video stream is ready
- Maintain decoder instance to minimize re-initialization overhead
- Log switch timing for debugging and performance monitoring

**Q&A**:

- Q: Does display switching restart the ICE connection?
  - A: No, ICE connection is preserved. Only video track is renegotiated.

- Q: Is there a complete stream pause during switching?
  - A: No, there is a brief frame gap (5-7 frames), but stream does not pause.

- Q: Can input be sent during display switch?
  - A: Yes, data channel remains active, but input actions may be delayed until new display is ready.

- Q: What happens if SDP renegotiation fails?
  - A: System attempts to restore previous display stream and displays error to user.

- Q: Can multiple clients switch to different displays simultaneously?
  - A: Yes, each client's switch is independent. Server maintains separate streams per client.

### Zoom and Pan Specification (U2)

**Purpose**: Allow mobile client users to zoom and pan the remote desktop view for better visibility of small UI elements. View manipulation is client-side only and does NOT affect the actual Windows desktop resolution or layout.

**Zoom Behavior**:

| Parameter | Value | Description |
|-----------|-------|-------------|
| Minimum Zoom | 0.5x | Cannot zoom out below 50% of original size |
| Maximum Zoom | 3.0x | Cannot zoom in above 300% of original size |
| Default Zoom | 1.0x | Original size (no zoom) |
| Zoom Granularity | Continuous | Pinch gesture controls zoom smoothly |
| Zoom Step (Buttons) | ±0.25x | +/- buttons adjust zoom in discrete steps |

**Pan Behavior**:

| Parameter | Value | Description |
|-----------|-------|-------------|
| Pan Bounds | Bounded | Cannot pan beyond desktop boundaries |
| Pan Threshold | 10px | Minimum drag distance before pan starts |
| Inertia | Enabled | Smooth deceleration after drag release |
| Reset Gesture | Double-tap | Double-tap resets zoom to 1.0x and centers view |

**Coordinate System**:
- Original (1.0x zoom): Desktop coordinates directly mapped to mobile screen
- Zoomed in (>1.0x): Mobile screen shows subset of desktop, centered on zoom point
- Zoomed out (<1.0x): Entire desktop fits in mobile screen with letterboxing

**Gesture Details**:

**Pinch-to-Zoom**:
- Two-finger pinch: Zoom in (fingers move apart)
- Two-finger spread: Zoom out (fingers move together)
- Zoom anchor point: Center of pinch gesture on desktop
- Minimum pinch distance: 20px (to distinguish from tap)
- Zoom factor calculation: `new_zoom = current_zoom * (current_distance / start_distance)`
- Clamped to [0.5x, 3.0x] range

**Pan (Drag)**:
- Single-finger drag: Pan view in zoomed-in state
- Inertia enabled: Continue panning with deceleration after drag release
- Inertia decay: 10% per frame (stops after ~20 frames)
- Pan only active when zoom > 1.0x

**Edge Cases**:
- At 1.0x zoom: Pan disabled (no effect)
- At <1.0x zoom: Pan disabled (entire desktop visible)
- At boundary: Clamp pan to prevent showing empty space outside desktop
- During zoom: Adjust pan to keep anchor point stable

**Reset Behavior**:
- Double-tap anywhere: Instant reset to 1.0x zoom, centered view
- Reset animation: 300ms smooth transition (linear easing)
- Reset disabled if already at 1.0x zoom and centered

**Performance Requirements**:
- Zoom gesture: Update at 60fps during pinch
- Pan gesture: Update at 60fps during drag
- Animation: 60fps smooth transitions
- Memory: < 10MB for zoomed frame buffers

**Error Handling**:
- Invalid zoom input: Clamp to [0.5x, 3.0x] range silently
- Pan outside bounds: Clamp to desktop boundary silently
- Gesture conflict: Prefer zoom over tap if pinch detected (>20px movement)

**Testing Scenarios**:
1. Zoom in from 1.0x to 3.0x, verify smooth scaling
2. Zoom out from 1.0x to 0.5x, verify letterboxing
3. Pan at 2.0x zoom, verify boundaries respected
4. Rapid pinch gestures, verify no jitter
5. Double-tap reset, verify instant reset to 1.0x
6. Pan inertia, verify smooth deceleration

### Session 2025-02-11

- Q: How should the system handle conflicting input from multiple clients? → A: FIFO (first-in-first-out) - Process inputs strictly in arrival order
- Q: What should happen when GPU hardware encoding is not available? → A: Attempt software encoding with performance warning
- Q: Should display switching cause frame loss or be seamless? → A: Brief interruption - Allow temporary frame drop during switch (5-7 frames at 60fps, ~83-116ms) via WebRTC SDP renegotiation without ICE restart
- Q: Does display switching restart the WebRTC connection? → A: No, ICE connection is preserved. Only the video track is renegotiated via SDP to switch display source
- Q: What is the reconnection retry policy after network interruption? → A: Retry indefinitely with exponential backoff (initial: 1s, maximum: 30s, multiplier: 2x)
- Q: Should zoom/pan gestures affect mobile view or Windows desktop? → A: Client-side only - Affects only mobile view, not actual Windows desktop

## Assumptions

- Windows host has GPU with H.264 hardware encoding capability (NVIDIA NVENC or Intel Quick Sync)
- Mobile device uses Chrome browser with WebRTC and H.264 support
- Both devices are on the same local area network with minimal latency
- Windows host is running Windows 10 or 11
- Network bandwidth is sufficient to support 1080p@60fps video stream (approximately 5-15Mbps)
- No firewall rules block the required ports (default: 8080)
- Users understand and accept the open access model (no authentication, trusted network only)
