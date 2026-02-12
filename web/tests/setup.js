// Jest test setup file
// This file runs before each test suite

// Mock WebRTC API if not available in jsdom
global.RTCPeerConnection = class MockRTCPeerConnection {
  constructor(config) {
    this.config = config;
  }

  createDataChannel(label, options) {
    return { label, options };
  }

  createOffer() { return Promise.resolve({ type: 'offer' }); }
  createAnswer() { return Promise.resolve({ type: 'answer' }); }
  setLocalDescription(desc) { return Promise.resolve(); }
  setRemoteDescription(desc) { return Promise.resolve(); }
  addTrack(track, streams) {}
  close() {}
};

global.RTCDataChannel = class MockRTCDataChannel {
  constructor(label, options) {
    this.label = label;
    this.options = options;
    this.readyState = 'open';
  }

  send(data) {}

  addEventListener(event, handler) {}
  removeEventListener(event, handler) {}

  close() {}
};

global.RTCSessionDescription = class {
  constructor(init) {
    Object.assign(this, init);
  }
};
