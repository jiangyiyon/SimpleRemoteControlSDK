# T037: DXGI 60fps Capture Loop Optimization Plan

## Overview
Implement DXGI screen capture with 60fps support and stable frame delivery.

## Current Status
- ✅ Basic DXGI capture implemented
- ✅ Capture loop implemented
- ✅ Frame callback mechanism
- ✅ Simplified unit tests (functional only, no performance tests)
- ⏳ Performance optimization deferred to final phase

## Architecture Decision
**Design Choice: Serial Execution (Capture → Encode)**

Reasoning:
- Simpler architecture, easier to maintain
- Lower memory footprint
- No need for frame buffer pool in serial mode
- Performance can be optimized without parallelization
- Parallelization can be added later if needed

---

## Implementation Plan (Functional First, Optimize Later)

### Phase 1: Basic Functionality ✅
**Goal**: Ensure DXGI capture works correctly with functional tests

#### Completed Tasks:
1. ✅ Basic DXGI capture implementation
2. ✅ Capture loop with target fps setting
3. ✅ Frame callback mechanism
4. ✅ Simplified unit tests (functional only)
   - InitializeWithDefaultDisplay
   - InitializeWithInvalidDisplay
   - StartWithoutInitialize
   - SetAndGetTargetFps
   - FrameCallback
   - StopAndRestartCapture
   - MultipleStops

---

### Phase 2: Error Handling ⏳
**Goal**: Robust error handling and recovery

#### Tasks:
1. DXGI_ACCESS_DENIED recovery (session switch, lock screen)
2. Automatic reinitialization on errors
3. Clean shutdown handling

---

### Phase 3: Performance Optimization ⏳ (DEFERRED TO END)
**Goal**: Achieve 60fps with stable timing

#### 3.1 Performance Profiling
- Add timing measurements to capture loop
- Identify bottlenecks
- Measure at different resolutions

#### 3.2 Capture Optimization
- DXGI timeout tuning
- Texture copy optimization
- Memory allocation optimization

#### 3.3 Frame Rate Control
- Precise timing with `sleep_until()`
- Frame drop tracking and reporting

---

## Test Plan

### Unit Tests ✅
- `InitializeWithDefaultDisplay`
- `InitializeWithInvalidDisplay`
- `StartWithoutInitialize`
- `SetAndGetTargetFps`
- `FrameCallback` - Verify frames are captured with valid data
- `StopAndRestartCapture`
- `MultipleStops`

### Integration Tests ⏳
- Capture + Encoder integration
- Test with different resolutions
- Test with different frame rates (30fps, 60fps)

### Performance Tests ⏳ (DEFERRED)
- Measure capture time per frame
- Measure encoding time per frame
- Measure memory usage over time
- Verify 60fps target

---

## Implementation Priorities

### Priority 1: Basic Functionality ✅
- ✅ DXGI capture implementation
- ✅ Capture loop
- ✅ Frame callback
- ✅ Simplified functional tests

### Priority 2: Error Handling ⏳
1. DXGI error recovery (session switch, lock screen)
2. Robust error handling
3. Automatic recovery

### Priority 3: Performance Optimization ⏳ (DEFERRED)
1. Performance profiling
2. DXGI timeout tuning
3. Memory allocation optimization
4. Precise frame rate control with `sleep_until()`
5. Frame drop tracking

---

## Success Criteria

### Functional Goals
- ✅ DXGI capture initializes correctly
- ✅ Frames are captured and delivered via callback
- ✅ Start/stop/restart works correctly
- ✅ No crashes during normal operation

### Performance Goals (After Optimization)
- **Frame Rate**: 60fps target
- **Frame Interval Stability**: Consistent delivery
- **Frame Drops**: Minimal under normal conditions
- **CPU Usage**: Acceptable on modern hardware
- **Memory Usage**: Reasonable for capture resolution

### Quality Goals
- No screen tearing
- No missing frames (except DXGI errors)
- Accurate timing (within ±2ms of target interval)

### Reliability Goals
- No crashes during 1+ hour continuous capture
- Automatic recovery from DXGI errors
- Clean shutdown

---

## Notes

### Removed Features
- ~~VideoFramePool~~: Not needed for serial execution
- ~~Zero-copy optimization~~: Not applicable in serial mode
- ~~Parallel capture/encode~~: Deferred until necessary

### Future Enhancements (If needed)
- Parallel capture and encode threads
- Frame buffer pool for parallel execution
- Hardware encoding (NVENC, QuickSync)
- GPU-based capture optimization

---

## Timeline Estimate

| Phase | Tasks | Estimate | Status |
|-------|-------|----------|--------|
| Phase 1 | Basic Functionality | 2 hours | ✅ Completed |
| Phase 2 | Error Handling | 2 hours | ⏳ Pending |
| Phase 3 | Performance Optimization | 4 hours | ⏳ Deferred |

**Total**: ~8 hours

---

## References
- Original T037 task
- DXGI Desktop Duplication API documentation
- Performance optimization deferred to final phase after all functional work completed
