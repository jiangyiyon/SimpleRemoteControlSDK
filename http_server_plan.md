# Task Plan: HTTP/WebSocket Server Implementation for Mobile Browser Access

## Goal
Enable mobile device to view PC desktop via `http://<PC_IP>:<PORT>` using browser

## Current Phase
Phase 2: HTTP Static File Server (In Progress)

## Phases

### Phase 1: Setup & Dependencies

**Purpose**: Integrate cpp-httplib library and prepare project structure

- [x] Download cpp-httplib library (single-header library)
  - Download URL: https://github.com/yhirose/cpp-httplib/releases
  - Target version: v0.15.3 (actual: v0.30.1)
  - Save to: `third_party/cpp-httplib/httplib.h`
- [x] Update CMakeLists.txt to include cpp-httplib
  - Add include directory to target
  - Ensure threading support (pthread on Windows)
  - Update Windows version to 0x0A00 (Windows 10+)
- [x] Create directory structure for server implementation
  - `ScreenStreamSDK/src/server/` - Server implementation
  - `ScreenStreamSDK/include/screensdk/server/` - Server interfaces
  - `tests/unit/server/` - Server unit tests
  - `tests/integration/` - Integration tests
- [x] Verify cpp-httplib compilation
  - Create minimal test program
  - Ensure no compile errors
- **Status**: complete (2026-02-15)

**Summary**: Successfully integrated cpp-httplib v0.30.1. Test program compiles and runs successfully. Directory structure ready for server implementation.

### Phase 2: HTTP Static File Server

**Purpose**: Serve web client files (HTML/CSS/JS) to mobile browser

- [ ] Design IHttpServer interface (pure virtual)
  - Method: `start(port)`
  - Method: `stop()`
  - Method: `isRunning()`
  - Method: `setRootDirectory(path)`
  - Callback: `onWebSocketConnection(handler)`
- [ ] Implement HttpServer using cpp-httplib
  - Static file serving from `web/` directory
  - MIME type support (HTML, CSS, JS, JSON)
  - CORS headers for mobile access
  - Logging configuration
- [ ] Implement HttpServer factory functions
  - `CreateHttpServer()` - C-style factory
  - `DestroyHttpServer()` - C-style destroy
- [ ] Write unit tests for HttpServer
  - Test: Start/stop server
  - Test: Serve static file (index.html)
  - Test: Serve CSS/JS files
  - Test: CORS headers
  - Test: Concurrent requests
- [ ] Integration test: Browser access test
  - Test: Access `http://localhost:8080` returns index.html
  - Test: All assets load correctly
- **Status**: pending

### Phase 3: WebSocket Signaling Server

**Purpose**: Handle WebRTC SDP/ICE exchange between client and server

- [ ] Design WebSocket message protocol
  - Message types: offer, answer, ice-candidate, error
  - JSON format for messages
  - Message ID for request/response matching
- [ ] Design ISignalingServer interface (pure virtual)
  - Method: `start(port, callback)`
  - Method: `stop()`
  - Method: `sendToClient(session_id, message)`
  - Method: `broadcast(message, exclude_session_id)`
  - Callbacks: `onOffer`, `onAnswer`, `onIceCandidate`, `onConnection`, `onDisconnection`
- [ ] Implement SignalingServer using cpp-httplib WebSocket
  - Handle new WebSocket connections
  - Parse incoming JSON messages
  - Route messages to appropriate handlers
  - Session management (assign unique session IDs)
  - Client-to-server message forwarding
  - Server-to-client message broadcasting
- [ ] Implement SignalingServer factory functions
  - `CreateSignalingServer()` - C-style factory
  - `DestroySignalingServer()` - C-style destroy
- [ ] Write unit tests for SignalingServer
  - Test: Start/stop server
  - Test: WebSocket connection establishment
  - Test: JSON message parsing
  - Test: Offer message handling
  - Test: Answer message handling
  - Test: ICE candidate forwarding
  - Test: Session management
  - Test: Multiple concurrent connections
- [ ] Integration test: WebSocket signaling test
  - Test: Client connects via WebSocket
  - Test: SDP offer/answer exchange
  - Test: ICE candidate exchange
- **Status**: pending

### Phase 4: RemoteDesktopServer Integration

**Purpose**: Combine all components (capture + encode + WebRTC + HTTP) into single server

- [ ] Design IRemoteDesktopServer interface (pure virtual)
  - Method: `start(config)`
  - Method: `stop()`
  - Method: `isRunning()`
  - Method: `getSessionList()`
  - Callback: `onNewSession`, `onSessionClosed`, `onError`
- [ ] Design RemoteDesktopServerConfig struct
  - HTTP port
  - WebSocket port (or same as HTTP)
  - Web root directory
  - Capture settings (display ID, resolution)
  - Encoder settings (hardware preference)
  - WebRTC config (STUN server)
- [ ] Implement RemoteDesktopServer
  - Integrate DxgiCapture
  - Integrate EncoderFactory
  - Integrate WebrtcTransport
  - Integrate HttpServer
  - Integrate SignalingServer
  - Manage sessions
  - Handle client lifecycle (connect → stream → disconnect)
  - Error handling and recovery
- [ ] Implement RemoteDesktopServer factory functions
  - `CreateRemoteDesktopServer()` - C-style factory
  - `DestroyRemoteDesktopServer()` - C-style destroy
- [ ] Write unit tests for RemoteDesktopServer
  - Test: Start/stop server
  - Test: Session creation
  - Test: Session lifecycle
  - Test: Error handling
- [ ] Integration test: End-to-end streaming test
  - Test: Browser connects to server
  - Test: Video stream starts
  - Test: Video frames received in browser
  - Test: Session cleanup on disconnect
- **Status**: pending

### Phase 5: Real Device Testing

**Purpose**: Verify functionality on actual mobile devices

- [ ] Prepare test environment
  - Ensure PC and phone on same network
  - Get PC IP address
  - Configure firewall (open HTTP port)
- [ ] Test on Android device
  - Open Chrome browser
  - Navigate to `http://<PC_IP>:8080`
  - Verify page loads correctly
  - Verify video stream appears
  - Check latency (should be <100ms)
- [ ] Test on iOS device
  - Open Safari browser
  - Navigate to `http://<PC_IP>:8080`
  - Verify page loads correctly
  - Verify video stream appears
  - Check latency
- [ ] Test on desktop browsers
  - Chrome on Windows
  - Edge on Windows
  - Firefox
- [ ] Document any issues
  - Browser compatibility issues
  - Network issues
  - Performance issues
- **Status**: pending

### Phase 6: Final Polish

**Purpose**: Clean up and prepare for integration with existing tasks

- [ ] Update documentation
  - Quickstart guide
  - API documentation
  - Troubleshooting guide
- [ ] Create startup script
  - `start_server.bat` - Start server with default config
  - `stop_server.bat` - Stop server
- [ ] Create configuration file
  - `config/server.json` - Server settings
- [ ] Add logging
  - Server startup/shutdown logs
  - Connection logs
  - Error logs
- [ ] Optimize performance
  - Connection pooling
  - Message batching
  - Memory optimization
- **Status**: pending

### Phase 7: Backlog Integration (After MVP)

**Purpose**: Integrate with remaining tasks from original plan

- [ ] T040-T042: Implement input processors
- [ ] T027-T032: Write additional unit/integration tests
- [ ] Update task_plan.md to reflect new structure
- **Status**: pending

## Key Questions

1. **HTTP vs WebSocket ports**: Use same port (upgrade from HTTP to WebSocket) or separate ports?
   - **Decision**: Use separate ports for simplicity (HTTP: 8080, WebSocket: 8081)

2. **cpp-httplib version**: Which version to use?
   - **Decision**: v0.15.3 (stable release from GitHub releases)

3. **Signaling protocol**: Use WebSocket-only or HTTP + WebSocket fallback?
   - **Decision**: WebSocket-only (simpler, better for real-time)

4. **Multi-client support**: Implement now or defer to Phase 7?
   - **Decision**: Implement basic multi-client support in Phase 4 (single stream, multiple viewers)

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| Use cpp-httplib library | Single-header, mature, easy to integrate, saves implementation time |
| Separate HTTP and WebSocket ports | Simpler implementation, easier debugging |
| WebSocket-only signaling | Better real-time performance, simpler protocol |
| Implement basic multi-client support | Multiple viewers is common use case |
| Follow Pure Virtual Interface Pattern | Consistent with existing codebase |

## Errors Encountered

| Error | Attempt | Resolution |
|-------|---------|------------|
|       | 1       |            |

## Notes

- Update phase status as you progress: pending → in_progress → complete
- Re-read this plan before major decisions
- Log ALL errors - they help avoid repetition
- Never repeat a failed action - mutate your approach instead
- Test each phase thoroughly before moving to next phase

## Technical Specifications

### cpp-httplib Integration

**Library**: cpp-httplib v0.15.3
**License**: MIT License
**Location**: `third_party/cpp-httplib/httplib.h`

**CMake Integration**:
```cmake
target_include_directories(screensdk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
)
```

**Required Features**:
- HTTP 1.1
- WebSocket (RFC 6455)
- Static file serving
- CORS support
- Thread safety

### WebSocket Message Protocol

**Message Format (JSON)**:
```json
{
  "type": "offer|answer|ice-candidate|error",
  "id": "unique-message-id",
  "session_id": "session-identifier",
  "data": {
    // Type-specific data
  }
}
```

**Message Types**:
- `offer`: SDP offer from client
- `answer`: SDP answer from server
- `ice-candidate`: ICE candidate exchange
- `error`: Error message

### Server Configuration

**Default Ports**:
- HTTP: 8080
- WebSocket: 8081

**Default Config**:
```json
{
  "http_port": 8080,
  "websocket_port": 8081,
  "web_root": "./web",
  "display_id": 0,
  "stun_server": "stun:stun.l.google.com:19302",
  "max_sessions": 4,
  "log_level": "info"
}
```
