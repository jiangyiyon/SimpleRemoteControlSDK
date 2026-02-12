# Contracts: LAN Low-Latency Remote Desktop SDK (Index)

**Feature**: 1-lan-remote-desktop
**Date**: 2025-02-11

## Contract Documents

This directory contains all API contracts for the Remote Desktop SDK implementation. Each contract defines an abstract interface that can be implemented for testing and production use.

### Core Contracts

1. **screen_capture.h** - IScreenCapture interface for screen capture and display enumeration
2. **video_encoder.h** - IVideoEncoder interface for H.264 encoding (hardware/software)
3. **webrtc_transport.h** - IWebrtcTransport interface for WebRTC peer-to-peer communication
4. **input_processor.h** - IInputProcessor interface for input event processing and Windows input mapping
5. **display_manager.h** - IDisplayManager interface for multi-display management and switching
6. **session_manager.h** - ISessionManager interface for multi-client session coordination (internal)

## Contract Usage

### For Implementers

1. Read the contract document for the interface you need to implement
2. Ensure all methods are implemented with exact signatures
3. Follow behavioral guarantees documented in each contract
4. Implement error handling as specified (error codes, callbacks)
5. Write unit tests against the contract interface (mocks/stubs allowed)

### For Testers

1. Create test fixtures using contract interfaces
2. Mock implementations for dependency injection
3. Verify behavioral guarantees through integration tests
4. Validate error handling for all documented error scenarios
5. Performance test against metric targets defined in contracts

### For Reviewers

1. Verify implementation matches contract signatures exactly
2. Check that behavioral guarantees are met
3. Ensure error handling follows contract specifications
4. Validate threading model compliance
5. Review performance metrics against targets

## Contract Evolution

Contracts may be updated based on:
- New feature requirements
- Performance optimization opportunities
- Bug fixes requiring interface changes
- Security considerations
- Platform-specific adjustments

**Process**:
1. Propose change with rationale
2. Update contract with version increment
3. Update all implementations
4. Update tests to match new contract
5. Document breaking changes in CHANGELOG.md

## Quick Reference

| Contract | Key Methods | Performance Target |
|-----------|-------------|-------------------|
| IScreenCapture | getNextFrame(), enumerateDisplays() | ≤ 5ms capture latency |
| IVideoEncoder | encode(), setTargetBitrate() | ≤ 10ms encoding latency |
| IWebrtcTransport | createOffer(), sendDataChannelMessage() | ≤ 5s connection time |
| IInputProcessor | processMouseEvent(), processQueue() | ≤ 5ms per event |
| IDisplayManager | switchDisplay(), selectDisplay() | ≤ 100ms switch time |
| ISessionManager | createSession(), routeInputEvent() | ≤ 4 concurrent sessions |
