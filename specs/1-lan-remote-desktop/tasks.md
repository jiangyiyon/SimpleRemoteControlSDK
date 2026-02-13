# Tasks: LAN Low-Latency Remote Desktop SDK

**Input**: Design documents from `/specs/1-lan-remote-desktop/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Server (C++)**: `ScreenStreamSDK/src/`, `ScreenStreamSDK/include/`, `ScreenStreamSDK/tests/`
- **Client (JavaScript)**: `web/src/`, `web/tests/`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [ ] T001 Create project directory structure per implementation plan at ScreenStreamSDK/, web/, tests/, docs/, config/, third_party/
- [ ] T002 Initialize CMakeLists.txt for C++20 project with CMake 3.15+ support in ScreenStreamSDK/CMakeLists.txt
- [ ] T003 Initialize JavaScript project with package.json and dependencies in web/package.json
- [ ] T004 [P] Clone and build libwebrtc source in third_party/webrtc/ (follow research.md build instructions)
- [ ] T005 [P] Download spdlog header-only library to third_party/spdlog/
- [ ] T006 [P] Download nlohmann/json header-only library to third_party/nlohmann/
- [ ] T007 [P] Create default configuration file template in config/default.json
- [ ] T008 Create C++ build configuration for Google Test in tests/CMakeLists.txt
- [ ] T009 Create JavaScript test configuration for Jest in web/package.json

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T010 Implement RAII-based logger wrapper using spdlog in ScreenStreamSDK/src/utils/logger.cpp and ScreenStreamSDK/include/screensdk/utils/logger.h
- [ ] T011 Implement thread-safe configuration loader using nlohmann/json in ScreenStreamSDK/src/api/config.cpp and ScreenStreamSDK/include/screensdk/api/config.h
- [ ] T012 [P] Implement error code definitions and exception classes in ScreenStreamSDK/include/screensdk/utils/error.h
- [ ] T013 [P] Implement metrics collection framework with atomic variables in ScreenStreamSDK/src/utils/metrics_collector.cpp and ScreenStreamSDK/include/screensdk/utils/metrics_collector.h
- [ ] T014 [P] Implement thread pool for async task execution in ScreenStreamSDK/src/utils/thread_pool.cpp and ScreenStreamSDK/include/screensdk/utils/thread_pool.h
- [ ] T015 Implement WebRTC manager wrapper with libwebrtc integration in ScreenStreamSDK/src/transport/webrtc_manager.cpp and ScreenStreamSDK/include/screensdk/transport/webrtc_manager.h
- [ ] T016 [P] Implement custom video source adapter for WebRTC track in ScreenStreamSDK/src/transport/video_source.cpp and ScreenStreamSDK/include/screensdk/transport/video_source.h
- [ ] T017 Implement WebRTC data channel wrapper for input transmission in ScreenStreamSDK/src/transport/data_channel.cpp and ScreenStreamSDK/include/screensdk/transport/data_channel.h
- [ ] T018 Implement HTTP/WebSocket signaling server for WebRTC SDP exchange in ScreenStreamSDK/src/transport/signaling_server.cpp and ScreenStreamSDK/include/screensdk/transport/signaling_server.h
- [ ] T019 [P] Implement DXGI screen capture initialization in ScreenStreamSDK/src/capture/dxgi_capture.cpp and ScreenStreamSDK/include/screensdk/capture/dxgi_capture.h
- [ ] T020 [P] Implement display enumeration via Windows Display API in ScreenStreamSDK/src/capture/display_detector.cpp and ScreenStreamSDK/include/screensdk/capture/display_detector.h
- [ ] T021 [P] Implement encoder factory with hardware encoder/software encoder selection in ScreenStreamSDK/src/encoding/encoder_factory.cpp and ScreenStreamSDK/include/screensdk/encoding/encoder_factory.h
- [ ] T022 [P] Implement low-latency encoder configuration (GOP=1, B-frames=0) in ScreenStreamSDK/src/encoding/encoder_config.cpp and ScreenStreamSDK/include/screensdk/encoding/encoder_config.h
- [ ] U1 [US1] Implement Intel QuickSync hardware encoder (MFXVideoENCODE) as hardware encoder option in ScreenStreamSDK/src/encoding/qsv_encoder.cpp and ScreenStreamSDK/include/screensdk/encoding/qsv_encoder.h
- [ ] U2 [US2] Implement SDP renegotiation handler to manage WebRTC session description protocol renegotiation for display switching in ScreenStreamSDK/src/transport/sdp_renegotiation.cpp and ScreenStreamSDK/include/screensdk/transport/sdp_renegotiation.h
- [ ] U3 [US1] Implement WebRTC connection error handling and recovery in ScreenStreamSDK/src/transport/connection_handler.cpp and ScreenStreamSDK/include/screensdk/transport/connection_handler.h
- [ ] T023 [P] Implement Windows SendInput wrapper for mouse events in ScreenStreamSDK/src/input/windows_input.cpp and ScreenStreamSDK/include/screensdk/input/windows_input.h
- [ ] T024 [P] Implement Windows SendInput wrapper for keyboard events in ScreenStreamSDK/src/input/keyboard_handler.cpp and ScreenStreamSDK/include/screensdk/input/keyboard_handler.h
- [ ] T025 Implement Windows Display API wrapper for display management in ScreenStreamSDK/src/platform/display_api.cpp and ScreenStreamSDK/include/screensdk/platform/display_api.h
- [ ] T026 [P] Implement GPU capability detection (NVENC/QuickSync) in ScreenStreamSDK/src/platform/gpu_detector.cpp and ScreenStreamSDK/include/screensdk/platform/gpu_detector.h

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Basic Remote Desktop Connection (Priority: P1) 🎯 MVP

**Goal**: Enable mobile device to connect and control Windows desktop via Chrome browser with ≤30ms latency

**Independent Test**: Connect mobile Chrome to Windows server on local network, verify desktop visible within 5 seconds, mouse/keyboard input responds within 30ms

### Tests for User Story 1 ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T027 [P] [US1] Unit test for Session entity initialization and state transitions in tests/unit/core/session_test.cpp
- [ ] T028 [P] [US1] Unit test for InputEvent serialization to JSON in tests/unit/input/input_event_test.cpp
- [ ] T029 [P] [US1] Unit test for VideoFrame encoding/decoding in tests/unit/encoding/video_frame_test.cpp
- [ ] U1 [P] [US1] Unit test for Intel QuickSync encoder initialization and encoding in tests/unit/encoding/qsv_encoder_test.cpp
- [ ] U3 [P] [US1] Unit test for WebRTC connection error handling and recovery in tests/unit/transport/connection_handler_test.cpp
- [ ] G1 [P] [US1] Integration test for hardware encoder failure to software encoder fallback in tests/integration/encoding_fallback_test.cpp
- [ ] T030 [P] [US1] Integration test for screen capture to encoder pipeline in tests/integration/capture_encoding_test.cpp
- [ ] T031 [P] [US1] Integration test for WebRTC peer connection establishment in tests/integration/transport_test.cpp
- [ ] T032 [P] [US1] End-to-end test for basic connection and input latency in tests/e2e/basic_connection_test.cpp
- [ ] T102 [US1] Stability test (24-hour continuous operation) in tests/e2e/stability_test.cpp - Run after User Story 1 completion

### Implementation for User Story 1

> **Note**: User Story 1 implements virtual keyboard input via mobile device's virtual keyboard interface. Touch gestures (pinch-zoom, pan, long-press, swipe) are implemented in User Story 3 and are separate from keyboard input handling.

- [ ] T033 [P] [US1] Implement Session class with state machine and latency tracking in ScreenStreamSDK/src/core/session.cpp and ScreenStreamSDK/include/screensdk/core/session.h
- [ ] T034 [P] [US1] Implement InputEvent data structures with JSON serialization in ScreenStreamSDK/src/input/input_event.cpp and ScreenStreamSDK/include/screensdk/input/input_event.h
- [ ] T035 [P] [US1] Implement VideoFrame data structures with reference counting in ScreenStreamSDK/src/capture/capture_pipeline.cpp and ScreenStreamSDK/include/screensdk/capture/capture_pipeline.h
- [ ] T036 [P] [US1] Implement SessionManager for single-client session lifecycle in ScreenStreamSDK/src/core/session_manager.cpp and ScreenStreamSDK/include/screensdk/core/session_manager.h
- [ ] T037 [US1] Implement DXGI screen capture loop at 60fps in ScreenStreamSDK/src/capture/dxgi_capture.cpp
- [ ] T038 [US1] Implement NVENC hardware encoder with low-latency settings in ScreenStreamSDK/src/encoding/nvenc_encoder.cpp and ScreenStreamSDK/include/screensdk/encoding/nvenc_encoder.h
- [ ] T039 [US1] Implement software H.264 encoder fallback (for systems without hardware encoder) in ScreenStreamSDK/src/encoding/software_encoder.cpp and ScreenStreamSDK/include/screensdk/encoding/software_encoder.h
- [ ] T040 [US1] Implement mouse event processor with coordinate mapping in ScreenStreamSDK/src/input/mouse_handler.cpp and ScreenStreamSDK/include/screensdk/input/mouse_handler.h
- [ ] T041 [US1] Implement keyboard event processor with modifier key support in ScreenStreamSDK/src/input/keyboard_handler.cpp
- [ ] T042 [US1] Implement InputProcessor with FIFO queue (≤1000 events) in ScreenStreamSDK/src/input/input_processor.cpp and ScreenStreamSDK/include/screensdk/input/input_processor.h
- [ ] T043 [US1] Implement IScreenCapture interface in ScreenStreamSDK/src/capture/dxgi_capture.cpp
- [ ] T044 [US1] Implement IVideoEncoder interface in ScreenStreamSDK/src/encoding/encoder_factory.cpp
- [ ] T045 [US1] Implement IWebrtcTransport interface in ScreenStreamSDK/src/transport/webrtc_manager.cpp
- [ ] T046 [US1] Implement IInputProcessor interface in ScreenStreamSDK/src/input/input_processor.cpp
- [ ] T047 [US1] Implement screensdk::createSession() public API in ScreenStreamSDK/src/api/screensdk.cpp and ScreenStreamSDK/include/screensdk/screensdk.h
- [ ] T048 [US1] Implement WebRTC client JavaScript class in web/src/client.js
- [ ] T049 [US1] Implement WebRTC connection establishment in web/src/webrtc_connection.js
- [ ] T050 [US1] Implement Canvas 2D video renderer in web/src/video_renderer.js
- [ ] T051 [US1] Implement touch/mouse input capture in web/src/input_capture.js
- [ ] T052 [US1] Implement latency measurement and display in web/src/metrics_display.js
- [ ] T053 [US1] Create main client HTML page in web/index.html
- [ ] T054 [US1] Add client CSS styling in web/styles/client.css

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - Multi-Display Switching (Priority: P2)

**Goal**: Enable mobile client to enumerate and switch between multiple Windows displays within 100ms

**Independent Test**: Connect to Windows host with 2+ displays, verify display list shows correctly, switch between displays and verify transition ≤100ms

### Tests for User Story 2 ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [x] T055 [P] [US2] Unit test for DisplaySource entity in tests/unit/capture/display_detector_test.cpp
- [x] U2 [P] [US2] Unit test for SDP renegotiation handler in tests/unit/transport/sdp_renegotiation_test.cpp
- [ ] T056 [P] [US2] Integration test for display enumeration in tests/integration/display_switch_test.cpp
- [ ] T057 [P] [US2] Integration test for display switch timing (≤100ms) in tests/integration/display_switch_test.cpp
- [ ] T057a [P] [US2] Integration test for display hot-plug (add/remove display during active session) in tests/integration/display_switch_test.cpp

### Implementation for User Story 2

- [x] T058 [P] [US2] Implement DisplaySource data structure in ScreenStreamSDK/src/capture/display_detector.cpp
- [ ] T059 [P] [US2] Implement IDisplayManager interface in ScreenStreamSDK/src/core/display_controller.cpp and ScreenStreamSDK/include/screensdk/core/display_controller.h
- [ ] T060 [US2] Implement display enumeration from DXGI in ScreenStreamSDK/src/capture/display_detector.cpp
- [ ] T061 [US2] Implement display selection per session in ScreenStreamSDK/src/core/display_controller.cpp
- [ ] T062 [US2] Implement display switch with SDP renegotiation for seamless transition (≤100ms) in ScreenStreamSDK/src/core/display_controller.cpp
- [ ] T063 [US2] Implement display configuration change detection in ScreenStreamSDK/src/core/display_controller.cpp
- [ ] T064 [US2] Add display_source_id to Session entity in ScreenStreamSDK/src/core/session.cpp
- [ ] T065 [US2] Implement display selector UI dropdown in web/src/display_selector.js
- [ ] T066 [US2] Update client to handle display switch via Web renegotiation in web/src/client.js
- [ ] T067 [US2] Add display information display to web UI in web/index.html
- [ ] T102 [US2] Stability test (24-hour continuous operation) in tests/e2e/stability_test.cpp - Re-run after User Story 2 completion

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - Touch Gesture Support (Priority: P3)

**Goal**: Enable mobile touch gestures (pinch-zoom, pan, long-press, swipe) for intuitive desktop interaction

**Independent Test**: Connect to Windows host, perform pinch-zoom to scale view, pan to scroll, long-press for right-click, verify all gestures work correctly

### Tests for User Story 3 ⚠️

> **NOTE: Write these tests FIRST, ensure them FAIL before implementation**

- [ ] T068 [P] [US3] Unit test for GestureEvent data structures in tests/unit/input/gesture_handler_test.cpp
- [ ] T069 [P] [US3] Integration test for gesture recognition in web/tests/client/gesture_recognizer_test.js

### Implementation for User Story 3

- [ ] T070 [P] [US3] Implement GestureEvent data structures in ScreenStreamSDK/src/input/gesture_handler.cpp and ScreenStreamSDK/include/screensdk/input/gesture_handler.h
- [ ] T071 [US3] Implement touch gesture recognizer (pinch, pan, long-press, swipe) in web/src/gesture_recognizer.js
- [ ] T072 [US3] Implement gesture event processor on server side in ScreenStreamSDK/src/input/gesture_handler.cpp
- [ ] T073 [US3] Implement view controller for zoom/pan (client-side only) in web/src/view_controller.js
- [ ] T074 [US3] Add aspect ratio handling for client orientation changes in web/src/view_controller.js
- [ ] T075 [US3] Update input capture to support touch events in web/src/input_capture.js
- [ ] T076 [US3] Map pinch-zoom to Ctrl+wheel on Windows in ScreenStreamSDK/src/input/gesture_handler.cpp
- [ ] T077 [US3] Map pan to scroll events on Windows in ScreenStreamSDK/src/input/gesture_handler.cpp
- [ ] T078 [US3] Map long-press to right-click on Windows in ScreenStreamSDK/src/input/gesture_handler.cpp
- [ ] T079 [US3] Map swipe to scroll wheel on Windows in ScreenStreamSDK/src/input/gesture_handler.cpp
- [ ] T102 [US3] Stability test (24-hour continuous operation) in tests/e2e/stability_test.cpp - Re-run after User Story 3 completion

**Checkpoint**: All user stories should now be independently functional

---

## Phase 6: User Story 4 - Multi-Client Support (Priority: P4)

**Goal**: Enable up to 4 simultaneous mobile client connections with FIFO input ordering

**Independent Test**: Connect 3 different mobile devices to same Windows host, verify all devices see desktop and can send input, confirm FIFO ordering

### Tests for User Story 4 ⚠️

> **NOTE: Write these tests FIRST, ensure them FAIL before implementation**

- [ ] T080 [P] [US4] Unit test for ClientConnection entity in tests/unit/core/session_manager_test.cpp
- [ ] T081 [P] [US4] Integration test for multi-client connection in tests/integration/multi_client_test.cpp
- [ ] T082 [P] [US4] Integration test for FIFO input ordering across clients in tests/integration/multi_client_test.cpp

### Implementation for User Story 4

- [ ] T083 [P] [US4] Implement ClientConnection data structure with metrics in ScreenStreamSDK/src/core/session_manager.cpp
- [ ] T084 [US4] Update SessionManager to support up to 4 sessions in ScreenStreamSDK/src/core/session_manager.cpp
- [ ] T085 [US4] Implement FIFO input router across all sessions in ScreenStreamSDK/src/core/input_router.cpp and ScreenStreamSDK/include/screensdk/core/input_router.h
- [ ] T086 [US4] Implement per-session display subscription in ScreenStreamSDK/src/core/session_manager.cpp
- [ ] T087 [US4] Add bandwidth estimation and packet loss tracking per client in ScreenStreamSDK/src/core/session_manager.cpp
- [ ] T088 [US4] Implement exponential backoff reconnection handler in ScreenStreamSDK/src/core/reconnection_handler.cpp and ScreenStreamSDK/include/screensdk/core/reconnection_handler.h
- [ ] T089 [US4] Update WebRTC transport to handle multiple peer connections in ScreenStreamSDK/src/transport/webrtc_manager.cpp
- [ ] T090 [US4] Add client connection count display to web UI in web/index.html
- [ ] T102 [US4] Final stability test (24-hour continuous operation) in tests/e2e/stability_test.cpp - Final validation after all user stories complete

**Checkpoint**: At this point, all 4 user stories should work together

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [ ] T091 [P] Write C++ API reference documentation in docs/API.md
- [ ] T092 [P] Write JavaScript API reference documentation in docs/JavaScript-API.md
- [ ] T093 [P] Write architecture overview documentation in docs/Architecture.md
- [ ] T094 [P] Write build instructions in docs/Building.md
- [ ] T095 [P] Write deployment guide in docs/Deployment.md
- [ ] T096 [P] Write performance tuning guide in docs/Performance.md
- [ ] T097 [P] Create README.md with project overview and quickstart link
- [ ] T098 [P] Add license file (Apache 2.0) to LICENSE
- [ ] T099 [P] Create CHANGELOG.md template with version history
- [ ] T100 [P] Create third_party_notices.txt with library attributions
- [ ] T101 [P] Add performance regression tests for latency and framerate in tests/e2e/latency_test.cpp
- [ ] T103 [P] Optimize memory usage and add leak detection
- [ ] T104 [P] Add code comments for complex algorithms
- [ ] T105 [P] Run all linters and fix warnings
- [ ] T106 [P] Scan dependencies for security vulnerabilities
- [ ] T107 Validate quickstart.md instructions work end-to-end
- [ ] T108 Create start.bat and stop.bat Windows scripts in bin/
- [ ] T109 Package SDK binaries and web client into distribution zip

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3-6)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P1 → P2 → P3 → P4)
- **Polish (Phase 7)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - Integrates with Session entity from US1 but independently testable
- **User Story 3 (P3)**: Can start after Foundational (Phase 2) - Integrates with InputProcessor from US1 but independently testable
- **User Story 4 (P4)**: Can start after Foundational (Phase 2) - Depends on SessionManager from US1, independently testable

### Within Each User Story

- Tests MUST be written and FAIL before implementation
- Data structures before services/components
- Core implementation before integration
- Story complete before moving to next priority

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel (T004-T009)
- All Foundational tasks marked [P] can run in parallel within Phase 2 (T012-T026)
- Once Foundational phase completes, all user stories can start in parallel (if team capacity allows)
- All tests for a user story marked [P] can run in parallel
- Data structures within a story marked [P] can run in parallel (e.g., T033-T035 for US1)
- Different user stories can be worked on in parallel by different team members (US1/US2/US3/US4)
- All documentation tasks in Phase 7 marked [P] can run in parallel (T091-T096)

---

## Parallel Example: User Story 1

```bash
# Launch all tests for User Story 1 together:
Task: "Unit test for Session entity initialization and state transitions in tests/unit/core/session_test.cpp"
Task: "Unit test for InputEvent serialization to JSON in tests/unit/input/input_event_test.cpp"
Task: "Unit test for VideoFrame encoding/decoding in tests/unit/encoding/video_frame_test.cpp"
Task: "Integration test for screen capture to encoder pipeline in tests/integration/capture_encoding_test.cpp"
Task: "Integration test for WebRTC peer connection establishment in tests/integration/transport_test.cpp"
Task: "End-to-end test for basic connection and input latency in tests/e2e/basic_connection_test.cpp"

# Launch all data structures for User Story 1 together:
Task: "Implement Session class with state machine and latency tracking in ScreenStreamSDK/src/core/session.cpp"
Task: "Implement InputEvent data structures with JSON serialization in ScreenStreamSDK/src/input/input_event.cpp"
Task: "Implement VideoFrame data structures with reference counting in ScreenStreamSDK/src/capture/capture_pipeline.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001-T009)
2. Complete Phase 2: Foundational (T010-T026) - CRITICAL - blocks all stories
3. Complete Phase 3: User Story 1 (T027-T054)
4. **STOP and VALIDATE**: Test User Story 1 independently with real device
5. Deploy/demo if ready

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 1 → Test independently → Deploy/Demo (MVP!)
3. Add User Story 2 → Test independently → Deploy/Demo
4. Add User Story 3 → Test independently → Deploy/Demo
5. Add User Story 4 → Test independently → Deploy/Demo
6. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together (Phase 1-2)
2. Once Foundational is done:
   - Developer A: User Story 1 (T027-T054)
   - Developer B: User Story 2 (T055-T067)
   - Developer C: User Story 3 (T068-T079)
   - Developer D: User Story 4 (T080-T090)
3. Stories complete and integrate independently
4. All developers contribute to Phase 7 Polish (T091-T109)

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing (TDD approach per constitution)
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Stability test (T102) runs after each user story completion to ensure no regressions
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence
- Total task count: 114 (109 base + 3 new implementation: U1, U2, U3 + 2 new tests: T057a, G1)
- Tasks per user story: US1 (31, includes U1, U3, G1, T102), US2 (15, includes U2, T057a, T102), US3 (13, includes T102), US4 (12, includes T102)
- Parallel opportunities: Significant - 68 tasks marked [P]
- MVP scope: Phases 1-3 (Tasks T001-T054 + U1 + U3 + G1 = 57 tasks)

## Recent Additions

- **U1**: Intel QuickSync hardware encoder (MFXVideoENCODE) - Implements hardware encoder for Intel GPUs as alternative hardware encoder when NVENC unavailable
- **U2**: SDP renegotiation handler - Manages WebRTC Session Description Protocol renegotiation for seamless display switching operations
- **U3**: WebRTC connection error handling - Implements error detection, recovery, and automatic reconnection logic with exponential backoff
- **G1**: Hardware encoder to software encoder fallback test - Integration test for FR-003 requirement: validates transition from NVENC/QuickSync to software H.264 encoder when hardware unavailable
