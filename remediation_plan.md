# Remediation Plan: HIGH Priority Issues
<!-- 
  WHAT: Detailed remediation plan for Phase 2
  WHY: Provides actionable steps to resolve HIGH priority issues identified in findings.md
  WHEN: Created during Phase 2 (Remediation Planning), updated as decisions are made
-->

## Overview
This document provides detailed remediation plans for all HIGH priority issues (I2-I5, U1-U2) identified during specification analysis.

---

## Issue I2: Contract File Implementation Mismatch

### Issue Description
- **Location**: plan.md:L143 vs tasks.md:T043
- **Severity**: HIGH
- **Type**: Inconsistency
- **Current State**: plan.md references `contracts/screen_capture.h` but actual implementation in tasks.md uses `IScreenCapture` in `dxgi_capture.cpp`
- **Impact**: Planned architecture doesn't match actual task implementation

### Analysis
The contract file `contracts/screen_capture.h` EXISTS and contains a well-defined `IScreenCapture` interface. However:
- The actual implementation `DxgiCapture` does NOT inherit from `IScreenCapture`
- `DxgiCapture` inherits from `IVideoSource` instead
- No adapter layer or contract implementation exists

### Root Cause
Task T043 description is misleading. It says "Implement IScreenCapture interface" but the actual implementation uses a different interface hierarchy.

### Recommended Remediation Options

#### Option A: Update Implementation to Match Contract (RECOMMENDED)
**Rationale**: Aligns with planned architecture, follows pure virtual interface pattern

**Steps**:
1. Create `IScreenCapture` interface in `ScreenStreamSDK/include/screensdk/capture/i_screen_capture.h`
2. Create `DxgiCaptureImpl` class implementing `IScreenCapture`
3. Wrap existing `DxgiCapture` functionality within `DxgiCaptureImpl`
4. Add factory functions `CreateScreenCapture()` / `DestroyScreenCapture()`
5. Update T043 description to reflect this architecture

**Pros**:
- Maintains contract consistency
- Follows Pure Virtual Interface Pattern (project rule)
- Better ABI stability
- Easier to mock for testing

**Cons**:
- Requires code refactoring
- Adds one layer of indirection

#### Option B: Update Contract to Match Implementation
**Rationale**: Minimizes code changes, follows existing implementation

**Steps**:
1. Update `contracts/screen_capture.h` to reflect `IVideoSource` interface
2. Remove `IScreenCapture` references from contract
3. Document the deviation from original plan

**Pros**:
- Minimal code changes
- Leverages existing implementation

**Cons**:
- Breaks contract-driven design
- Loses interface abstraction
- May cause confusion

### Decision
**Adopt Option A**: Update implementation to match contract

**Implementation Priority**: MEDIUM (can be addressed after core functionality)

---

## Issue I3: SendInput Integration Unclear

### Issue Description
- **Location**: spec.md:L100 vs tasks.md:T023
- **Severity**: HIGH
- **Type**: Inconsistency
- **Current State**: spec.md requires keyboard input but tasks.md only mentions `keyboard_handler.cpp` in T024
- **Impact**: Unclear if SendInput wrapper is properly implemented

### Analysis
Tasks T023 and T024 both mention "SendInput wrapper":
- T023: "Windows SendInput wrapper for mouse events" in `windows_input.cpp`
- T024: "Windows SendInput wrapper for keyboard events" in `keyboard_handler.cpp`

Both tasks implement SendInput wrappers for different input types.

### Root Cause
Task descriptions are accurate but findings.md misinterpreted them as duplicate.

### Recommended Remediation
**Action**: Mark I3 as RESOLVED in findings.md with clarification

**Clarification**:
- T023 implements `windows_input.cpp` for mouse events
- T024 integrates SendInput in `keyboard_handler.cpp` for keyboard events
- Both are correct implementations, not duplicates

**Update Required**:
```markdown
**I3: SendInput Integration Unclear** ✅ RESOLVED
- Location: spec.md:L100 vs tasks.md:T023
- Issue: spec.md requires keyboard input but tasks.md only mentions keyboard_handler.cpp
- Impact: Unclear if SendInput wrapper is implemented
- **Resolution**: SendInput is correctly implemented in two separate files:
  - T023: windows_input.cpp (mouse events)
  - T024: keyboard_handler.cpp (keyboard events)
  - Both tasks fulfill keyboard and mouse input requirements
```

### Decision
**Mark I3 as RESOLVED** - No code changes needed, just documentation update

---

## Issue I4: FPS Resolution Scope Unclear

### Issue Description
- **Location**: spec.md:L94 vs plan.md:L18
- **Severity**: HIGH
- **Type**: Inconsistency
- **Current State**: spec.md requires 60fps @ any resolution but plan.md only specifies 1080p
- **Impact**: Unclear if 60fps requirement applies to all resolutions

### Analysis
- **spec.md FR-001**: "System MUST capture screen content from Windows desktop at 60±5 frames per second"
- **plan.md Performance Requirements**: "Target 60fps at 1080p resolution"
- **Constitution**: No resolution-specific FPS requirements

### Root Cause
Plan.md specified a reference resolution (1080p) for performance targets, but didn't clarify if FPS requirement scales with resolution.

### Recommended Remediation Options

#### Option A: Clarify FPS is Resolution-Independent (RECOMMENDED)
**Rationale**: Matches spec.md intent, simpler to understand and test

**Update Required**:
Add to spec.md:
```markdown
### Clarifications
**Q: Does the 60fps requirement apply to all resolutions?**
A: Yes, the 60±5 fps requirement is resolution-independent. The system should maintain 
60fps at any supported resolution (e.g., 720p, 1080p, 1440p). Performance may vary by 
resolution, but the minimum 55 fps threshold applies across all resolutions.
```

**Add to plan.md Performance Requirements**:
```markdown
**Video Performance**
- Target 60fps at 1080p resolution (reference resolution)
- Minimum 55fps at any resolution (720p, 1080p, 1440p, 4K)
- Frame capture latency: ≤ 5ms
```

#### Option B: Define Resolution-Specific FPS Targets
**Rationale**: More realistic, acknowledges hardware limitations

**Update Required**:
```markdown
**Video Performance by Resolution**
| Resolution | Target FPS | Minimum FPS |
|-------------|------------|-------------|
| 720p        | 60         | 55          |
| 1080p       | 60         | 55          |
| 1440p       | 60         | 50          |
| 4K          | 30         | 25          |
```

**Pros**:
- Realistic expectations
- Hardware-aware design

**Cons**:
- More complex specification
- More tests required
- Potential customer confusion

### Decision
**Adopt Option A**: Clarify FPS is resolution-independent

**Implementation Priority**: HIGH (should be resolved before performance testing)

---

## Issue I5: Display Enumeration Potential Duplication

### Issue Description
- **Location**: tasks.md:T020 vs T060
- **Severity**: HIGH
- **Type**: Inconsistency
- **Current State**: Both T020 and T060 mention display enumeration
- **Impact**: Unclear separation of responsibilities

### Analysis
- **T020** (Phase 2): "Implement display enumeration via Windows Display API"
  - File: `display_detector.cpp` and `display_detector.h`
  - Purpose: Foundational display enumeration infrastructure
  
- **T060** (Phase 4, US2): "Implement display enumeration from DXGI"
  - File: `display_detector.cpp` (same file!)
  - Purpose: User Story 2 specific display enumeration

### Root Cause
Tasks appear duplicated because they target the same file. However:
- T020 is foundational (Phase 2, must complete before US2)
- T060 adds DXGI-specific enhancements for US2

### Current Implementation Status
Let's verify actual implementation:
- Both tasks reference `display_detector.cpp`
- Need to check if T020 and T060 are truly separate or incremental

### Recommended Remediation Options

#### Option A: Merge Tasks (RECOMMENDED)
**Rationale**: Eliminates confusion, acknowledges incremental development

**Action**:
1. Keep T020 as foundational display enumeration (Windows Display API)
2. Update T060 description to clarify it's an **extension** or **enhancement**
3. Or merge T060 into T020 if functionality is identical

**Updated T060 Description**:
```markdown
- [x] T060 [US2] Extend display enumeration with DXGI integration in ScreenStreamSDK/src/capture/display_detector.cpp
  * Note: Builds upon T020 foundation, adds DXGI-specific capabilities for multi-display support
```

#### Option B: Clarify Task Responsibilities
**Rationale**: Maintain task granularity but add clarification

**Updated Descriptions**:
```markdown
T020 [P] (Phase 2): Implement basic display enumeration via Windows Display API
* Implement DisplaySource entity
* Enumerate connected displays
* Get display name, resolution, and primary display status

T060 [US2] (Phase 4): Implement DXGI integration for real-time display changes
* Add DXGI display enumeration for hot-plug detection
* Support display configuration change detection
* Integrate with display switching (T061-T062)
```

### Decision
**Adopt Option B**: Clarify task responsibilities with updated descriptions

**Implementation Priority**: LOW (documentation only, no code changes)

---

## Issue U1: Latency Measurement Methodology

### Issue Description
- **Location**: spec.md:L99
- **Severity**: HIGH
- **Type**: Underspecification
- **Current State**: FR-005 requires 30ms latency but no measurement methodology
- **Impact**: Impossible to verify requirement compliance without defined measurement approach

### Analysis
**FR-005**: "System MUST transmit input commands from mobile client to Windows host with one-way latency of 30ms or less (action → display)"

Key Questions:
1. **Where is "action" measured?**
   - Touch event timestamp on mobile client?
   - Input transmission timestamp?
   - Input arrival at Windows host?

2. **Where is "display" measured?**
   - Frame capture timestamp at Windows host?
   - Network transmission completion?
   - Frame render timestamp on mobile client?

3. **What constitutes "latency"?**
   - Round-trip time (RTT)?
   - One-way delay (OWD)?
   - End-to-end latency?

### Recommended Remediation

#### Define Latency Measurement Methodology

**Latency Definition**:
End-to-end one-way latency = Time from user action to visual feedback on screen

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

**Add to spec.md**:
```markdown
### Latency Measurement Methodology

**Definition**: End-to-end one-way latency measures the time from user action (T1) to 
visual feedback display (T8).

**Measurement Points**:
1. **T1**: User action timestamp (touch/click event on mobile client)
   - Measured using `performance.now()` on touch/click event
   - Precision: ±1ms (sub-millisecond on modern browsers)

2. **T2**: Input transmission timestamp (WebRTC data channel send)
   - Timestamped just before calling `dataChannel.send()`
   - Included in input event message payload

3. **T3**: Network transmission (client → host)
   - Implicit: Time between T2 and T4 (includes network RTT/2)
   - Estimated: (T4 - T2) / 2 (assuming symmetric network)

4. **T4**: Input processing timestamp (Windows host receives input)
   - Timestamped when input event is received from data channel
   - Includes `T1` and `T2` from client payload for round-trip calculation

5. **T5**: Windows response (input processed, desktop updated)
   - Implicit: After SendInput() API call completes
   - Measured as time from T4 to T5

6. **T6**: Frame capture timestamp (video frame encoding)
   - Timestamped when frame is captured from display
   - Includes `T1` from client payload

7. **T7**: Network transmission (host → client)
   - Implicit: Time between T6 and T8 (includes network RTT/2)

8. **T8**: Display timestamp (frame rendered on mobile client)
   - Measured using `requestAnimationFrame` callback timestamp
   - Precision: ±1ms (synchronized with display refresh)

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
- Baseline test: 100 consecutive clicks, measure average, P50, P95, P99
- Stress test: Rapid clicks (10 clicks/second), verify no latency degradation
- Network test: Simulate 10ms, 20ms, 50ms RTT, verify latency scales linearly
- Multi-client test: 4 clients simultaneously, verify no latency degradation

**Acceptance Criteria**:
- Average one-way latency ≤ 30ms (P50)
- 95th percentile latency ≤ 40ms (P95)
- 99th percentile latency ≤ 50ms (P99)
- Maximum latency spike < 100ms (except network interruption)
```

### Decision
**Add comprehensive latency measurement methodology to spec.md**

**Implementation Priority**: HIGH (required for testing FR-005)

---

## Issue U2: Zoom/Pan Bounds Undefined

### Issue Description
- **Location**: spec.md:L104
- **Severity**: HIGH
- **Type**: Underspecification
- **Current State**: FR-012 mentions "view manipulation only" but doesn't define zoom/pan bounds
- **Impact**: Unclear implementation constraints, potential for inconsistent behavior

### Analysis
**FR-012**: "System MUST support pinch-to-zoom and pan gestures on mobile client for view manipulation only (does not affect actual Windows desktop)"

Key Questions:
1. **Zoom limits**: Minimum zoom level? Maximum zoom level?
2. **Pan boundaries**: Can user pan off-screen? Infinite canvas or bounded?
3. **Zoom granularity**: Continuous zoom or discrete levels?
4. **Animation**: Smooth transitions or instant changes?
5. **Reset gesture**: Double-tap to reset? Button to reset?

### Recommended Remediation

#### Define Zoom/Pan Behavioral Specifications

**Add to spec.md**:
```markdown
### Zoom and Pan Specification (FR-012)

**Purpose**: Allow mobile client users to zoom and pan the remote desktop view for better 
visibility of small UI elements. View manipulation is client-side only and does NOT affect 
the actual Windows desktop resolution or layout.

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
```

### Decision
**Add comprehensive zoom/pan specification to spec.md**

**Implementation Priority**: MEDIUM (can be implemented after core functionality)

---

## Priority Summary

### Immediate Action Required (HIGH Priority)
1. **I3**: Update findings.md (documentation only)
2. **I4**: Clarify FPS resolution scope (documentation only)
3. **U1**: Define latency measurement methodology (documentation only)

### Medium Priority (Can Defer)
4. **I2**: Contract vs implementation alignment (code refactoring)
5. **I5**: Clarify display enumeration tasks (documentation only)
6. **U2**: Define zoom/pan bounds (documentation only)

### Documentation Updates Required
- findings.md: Mark I3 as RESOLVED
- spec.md: Add latency measurement methodology (U1)
- spec.md: Add zoom/pan specification (U2)
- spec.md: Clarify FPS resolution scope (I4)
- tasks.md: Clarify T020/T060 separation (I5)

### Code Changes Required
- I2: Create `IScreenCapture` interface and adapter (deferred to Phase 3)

---

## Next Steps

1. **Execute documentation updates** (all HIGH priority issues except I2)
2. **Review and approve plan** with user
3. **Implement code changes** for I2 (if approved)
4. **Proceed to Phase 3**: Remediation Execution

---

## Dependencies

- **U1 (Latency Measurement)** must be resolved before FR-005 testing
- **I4 (FPS Scope)** must be resolved before performance testing
- **U2 (Zoom/Pan)** must be resolved before FR-012 implementation
- **I2 (Contract Alignment)** can be deferred until after core functionality

---

## Questions for User

1. **I2**: Do you agree with Option A (align implementation to contract) or Option B (align contract to implementation)?
2. **U1**: Is the proposed latency measurement methodology acceptable?
3. **U2**: Are the zoom limits (0.5x - 3.0x) and bounds appropriate for your use case?
4. **I4**: Should FPS be resolution-independent or resolution-specific?
5. **Priority**: Should all HIGH priority issues be resolved before Phase 3, or can MEDIUM priority be deferred?

---

*Document Version: 1.0*
*Last Updated: 2026-02-14*
