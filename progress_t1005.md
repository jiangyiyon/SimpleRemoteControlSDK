# T1005: WebSocket Signaling Implementation Progress

## Task Overview
Implement WebSocket-based signaling server using libdatachannel to enable real-time WebRTC peer connections between browser clients and RemoteDesktopServer.

## Status: Planning Phase ✅

## Implementation Plan

### Phase 1: Architecture Design (Not Started)
- [ ] Design WebSocket signaling message protocol
- [ ] Design integration between SignalingServer and RemoteDesktopServer
- [ ] Design client-side WebSocket integration

### Phase 2: Server-Side Implementation (Not Started)
- [ ] Replace SignalingServer HTTP POST with libdatachannel WebSocket
  - [ ] Create WebSocketServer using rtc::WebSocketServer
  - [ ] Implement WebSocket connection handling
  - [ ] Implement message routing to RemoteDesktopServer
- [ ] Implement SDP offer/answer exchange
  - [ ] Receive client offer via WebSocket
  - [ ] Route to RemoteDesktopServer WebRTC Manager
  - [ ] Create and send answer via WebSocket
- [ ] Implement ICE candidate exchange
  - [ ] Receive client ICE candidates
  - [ ] Forward to RemoteDesktopServer WebRTC Manager
  - [ ] Send server ICE candidates to client
- [ ] Integration testing

### Phase 3: Client-Side Implementation (Not Started)
- [ ] Update web client to use WebSocket API
  - [ ] Replace fetch() with WebSocket
  - [ ] Implement connection handling
  - [ ] Implement offer/answer exchange
  - [ ] Implement ICE candidate exchange
- [ ] Update client.js signaling logic
- [ ] Testing with browser

### Phase 4: End-to-End Testing (Not Started)
- [ ] Integration test: Full WebRTC connection
- [ ] Test on desktop browsers (Chrome, Edge, Firefox)
- [ ] Test on mobile devices (Android, iOS)
- [ ] Performance testing
- [ ] Completion report

## Technical Decisions

### Why libdatachannel WebSocket?
1. **Built-in WebSocket support**: libdatachannel provides rtc::WebSocketServer
2. **Low latency**: Real-time bidirectional communication
3. **No polling overhead**: Eliminates HTTP POST polling delays
4. **Standard approach**: Common WebRTC signaling pattern
5. **Existing dependency**: No new dependencies required

### Message Protocol
```json
// Offer (Client → Server)
{
  "type": "offer",
  "sdp": "v=0\r\n..."
}

// Answer (Server → Client)
{
  "type": "answer",
  "sdp": "v=0\r\n..."
}

// ICE Candidate (Bidirectional)
{
  "type": "ice-candidate",
  "candidate": "candidate:...",
  "sdpMid": "0",
  "sdpMLineIndex": 0
}
```

## Dependencies
- libdatachannel (already integrated)
- cpp-httplib (for static file serving)
- RemoteDesktopServer (for WebRTC integration)

## Risks & Mitigations
| Risk | Mitigation |
|------|------------|
| WebSocket server complexity | Start with simple echo server, add routing incrementally |
| Integration with RemoteDesktopServer | Define clear callback interface |
| Client-side WebSocket API | Use standard browser WebSocket API |
| ICE candidate timing | Use WebRTC icecandidate event listener |

## References
- libdatachannel WebSocket API: third_party/webrtc/libdatachannel_x64-windows/include/rtc/websocketserver.hpp
- Current SignalingServer: ScreenStreamSDK/src/server/signaling_server.cpp
- RemoteDesktopServer: ScreenStreamSDK/src/server/remote_desktop_server.cpp
- Web Client: web/src/webrtc_connection.js
