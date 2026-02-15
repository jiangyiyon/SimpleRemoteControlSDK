/**
 * @file client.js
 * @brief Main WebRTC client class for remote desktop
 *
 * Integrates all remote desktop client components:
 * - WebRTC connection management
 * - Video rendering
 * - Input capture and transmission
 * - Latency measurement
 * - Session management
 *
 * @module RemoteDesktopClient
 */

'use strict';

// Import modules (in browser environment, these will be loaded via script tags)
// For Node/CommonJS environment:
// const WebrtcConnection = require('./webrtc_connection');
// const VideoRenderer = require('./video_renderer');
// const InputCapture = require('./input_capture');
// const MetricsDisplay = require('./metrics_display');
// const DisplaySelector = require('./display_selector');

/**
 * @classdesc Remote desktop client
 * @class
 */
class RemoteDesktopClient {
  /**
   * @constructor
   * @param {Object} config - Client configuration
   * @param {string} config.signalingUrl - Signaling server WebSocket URL
   * @param {HTMLElement} config.videoContainer - Container for video renderer
   * @param {HTMLElement} config.inputTarget - Target element for input capture
   * @param {Object} config.rtcConfig - WebRTC configuration
   * @param {Object} config.screenDims - Server screen dimensions {width, height}
   * @param {boolean} config.showMetrics - Show metrics display (default: true)
   * @param {HTMLElement} config.displaySelectorContainer - Container for display selector
   * @param {boolean} config.enableDisplaySwitch - Enable display switching (default: true)
   */
  constructor(config = {}) {
    this.signalingUrl = config.signalingUrl || 'ws://localhost:8080';
    this.videoContainer = config.videoContainer || document.body;
    this.inputTarget = config.inputTarget || document.body;
    this.rtcConfig = config.rtcConfig || {
      iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
    };
    this.screenDims = config.screenDims || { width: 1920, height: 1080 };
    this.showMetrics = config.showMetrics !== undefined ? config.showMetrics : true;
    this.displaySelectorContainer = config.displaySelectorContainer || null;
    this.enableDisplaySwitch = config.enableDisplaySwitch !== undefined ? config.enableDisplaySwitch : true;

    this.webrtcConnection = null;
    this.videoRenderer = null;
    this.inputCapture = null;
    this.metricsDisplay = null;
    this.displaySelector = null;
    this.signalingSocket = null;

    this.connected = false;
    this.sessionId = null;
    this.currentDisplayId = null;

    this.onConnected = null;
    this.onDisconnected = null;
    this.onError = null;

    this.logger = this._createLogger();
  }

  /**
   * @brief Create console logger with prefix
   * @private
   */
  _createLogger() {
    return {
      info: (...args) => console.log('[RemoteDesktopClient]', ...args),
      warn: (...args) => console.warn('[RemoteDesktopClient]', ...args),
      error: (...args) => console.error('[RemoteDesktopClient]', ...args)
    };
  }

  /**
   * @brief Initialize client components
   * @returns {Promise<void>}
   */
  async initialize() {
    try {
      this.logger.info('Initializing remote desktop client...');

      // Initialize WebRTC connection
      this.webrtcConnection = new WebrtcConnection({
        signalingUrl: this.signalingUrl,
        rtcConfig: this.rtcConfig
      });

      this._setupWebrtcCallbacks();

      await this.webrtcConnection.initialize();

      // Initialize video renderer
      const canvas = document.createElement('canvas');
      canvas.className = 'remote-desktop-canvas';
      this.videoContainer.appendChild(canvas);

      this.videoRenderer = new VideoRenderer({
        canvas: canvas,
        targetFps: 60,
        smoothScaling: false
      });

      this._setupVideoRendererCallbacks();

      // Initialize input capture
      this.inputCapture = new InputCapture({
        target: this.inputTarget,
        onInput: (event) => this._handleInputEvent(event),
        screenDims: this.screenDims
      });

      // Initialize metrics display
      if (this.showMetrics) {
        this.metricsDisplay = new MetricsDisplay({
          container: document.body,
          updateInterval: 1000
        });
      }

      // Initialize display selector
      if (this.enableDisplaySwitch && this.displaySelectorContainer) {
        this.displaySelector = new DisplaySelector(
          this.displaySelectorContainer,
          {
            onDisplaySelect: (display) => this._handleDisplaySwitch(display),
            getDisplayList: () => this._getDisplayList(),
            getDisplayInfo: () => this._getDisplayInfo()
          }
        );
        this.logger.info('Display selector initialized');
      }

      this.logger.info('Client initialized successfully');
    } catch (error) {
      this.logger.error('Failed to initialize client:', error);
      this._onError(error);
      throw error;
    }
  }

  /**
   * @brief Setup WebRTC connection callbacks
   * @private
   */
  _setupWebrtcCallbacks() {
    this.webrtcConnection.onTrack = (stream) => {
      this.logger.info('Received video stream');
      this.videoRenderer.setStream(stream);
    };

    this.webrtcConnection.onDataChannelOpen = () => {
      this.logger.info('Data channel opened');
      this.inputCapture.enable();

      // Load display list after data channel is ready
      if (this.displaySelector) {
        this.logger.info('Loading display list...');
        this.displaySelector.loadDisplayList().catch(error => {
          this.logger.warn('Failed to load display list:', error);
        });
      }
    };

    this.webrtcConnection.onDataChannelMessage = (message) => {
      this._handleDataChannelMessage(message);
    };

    this.webrtcConnection.onDataChannelClose = () => {
      this.logger.info('Data channel closed');
      this.inputCapture.disable();
    };

    this.webrtcConnection.onConnectionStateChange = (state) => {
      this.logger.info('Connection state changed:', state);
      this.metricsDisplay.setState(state);

      if (state === 'connected') {
        this.connected = true;
        if (this.onConnected) {
          this.onConnected();
        }
      } else if (state === 'disconnected' || state === 'failed') {
        this.connected = false;
        if (this.onDisconnected) {
          this.onDisconnected();
        }
      }
    };

    this.webrtcConnection.onError = (error) => {
      this.logger.error('WebRTC connection error:', error);
      this._onError(error);
    };
  }

  /**
   * @brief Setup video renderer callbacks
   * @private
   */
  _setupVideoRendererCallbacks() {
    this.videoRenderer.onFrameRendered = () => {
      this.metricsDisplay.recordFrame();
    };

    this.videoRenderer.onFpsUpdate = (fps) => {
      this.metricsDisplay.updateFps(fps);
    };
  }

  /**
   * @brief Connect to remote desktop server
   * @returns {Promise<void>}
   */
  async connect() {
    try {
      this.logger.info('Connecting to server...');

      // Create SDP offer
      await this.webrtcConnection.createOffer();
      this.logger.info('Offer created');

      // In a real implementation, exchange SDP and ICE candidates via signaling server
      // For now, we'll just create the offer and wait for manual signaling
      this.logger.info('Ready to exchange SDP and ICE candidates');

    } catch (error) {
      this.logger.error('Failed to connect:', error);
      this._onError(error);
      throw error;
    }
  }

  /**
   * @brief Handle remote SDP offer
   * @param {string} sdp - Remote SDP offer
   * @returns {Promise<string>} SDP answer
   */
  async handleRemoteOffer(sdp) {
    try {
      this.logger.info('Handling remote offer...');

      await this.webrtcConnection.setRemoteDescription(sdp, 'offer');

      const answer = await this.webrtcConnection.createAnswer();
      this.logger.info('Answer created');

      return answer;
    } catch (error) {
      this.logger.error('Failed to handle remote offer:', error);
      throw error;
    }
  }

  /**
   * @brief Handle remote SDP answer
   * @param {string} sdp - Remote SDP answer
   */
  async handleRemoteAnswer(sdp) {
    try {
      this.logger.info('Handling remote answer...');

      await this.webrtcConnection.setRemoteDescription(sdp, 'answer');
      this.logger.info('Remote description set');
    } catch (error) {
      this.logger.error('Failed to handle remote answer:', error);
      throw error;
    }
  }

  /**
   * @brief Handle remote ICE candidate
   * @param {Object} candidate - ICE candidate
   */
  async handleIceCandidate(candidate) {
    try {
      await this.webrtcConnection.addIceCandidate(candidate);
    } catch (error) {
      this.logger.error('Failed to add ICE candidate:', error);
    }
  }

  /**
   * @brief Handle input event from capture
   * @private
   */
  _handleInputEvent(event) {
    // Record input timestamp for latency measurement
    this.metricsDisplay.recordInput();

    // Convert event to JSON and send via data channel
    const data = JSON.stringify(event);
    const success = this.webrtcConnection.sendData(data);

    // Record data throughput
    if (success) {
      this.metricsDisplay.recordData(data.length, 'sent');
    }

    if (!success) {
      this.logger.warn('Failed to send input event');
    }
  }

  /**
   * @brief Disconnect from server
   */
  disconnect() {
    this.logger.info('Disconnecting...');

    if (this.inputCapture) {
      this.inputCapture.disable();
    }

    if (this.videoRenderer) {
      this.videoRenderer.stop();
    }

    if (this.webrtcConnection) {
      this.webrtcConnection.close();
    }

    this.connected = false;

    if (this.onDisconnected) {
      this.onDisconnected();
    }

    this.logger.info('Disconnected');
  }

  /**
   * @brief Enable/disable metrics display
   * @param {boolean} show - Show metrics
   */
  showMetricsDisplay(show) {
    if (this.metricsDisplay) {
      this.metricsDisplay.show(show);
    }
  }

  /**
   * @brief Update server screen dimensions
   * @param {number} width - Screen width
   * @param {number} height - Screen height
   */
  updateScreenDimensions(width, height) {
    this.screenDims = { width, height };
    if (this.inputCapture) {
      this.inputCapture.setScreenDimensions(width, height);
    }
    if (this.videoRenderer) {
      this.videoRenderer.resizeCanvas();
    }
  }

  /**
   * @brief Get connection state
   * @returns {string} Connection state
   */
  getConnectionState() {
    return this.webrtcConnection ? this.webrtcConnection.getConnectionState() : 'new';
  }

  /**
   * @brief Check if connected
   * @returns {boolean} Connected status
   */
  isConnected() {
    return this.connected;
  }

  /**
   * @brief Get performance metrics
   * @returns {Object} Performance metrics
   */
  getMetrics() {
    return {
      latency: this.metricsDisplay.getLatencyStats(),
      rtt: this.metricsDisplay.getRttStats(),
      fps: this.videoRenderer.getFps(),
      dimensions: this.videoRenderer.getDimensions()
    };
  }

  /**
   * @brief Handle display switch request
   * @private
   * @param {Object} display - Display to switch to
   */
  async _handleDisplaySwitch(display) {
    try {
      this.logger.info('Switching to display:', display.id);

      if (!this.connected) {
        throw new Error('Cannot switch display: not connected');
      }

      // Send display switch request via data channel
      const switchRequest = {
        type: 'display_switch',
        display_id: display.id,
        timestamp: Date.now()
      };

      const success = this.webrtcConnection.sendData(JSON.stringify(switchRequest));
      if (!success) {
        throw new Error('Failed to send display switch request');
      }

      this.currentDisplayId = display.id;
      this.logger.info('Display switch request sent');

    } catch (error) {
      this.logger.error('Failed to switch display:', error);
      // Revert selection
      if (this.displaySelector) {
        this.displaySelector.selectDisplay(this.currentDisplayId);
      }
      this._onError(error);
    }
  }

  /**
   * @brief Get list of available displays from server
   * @private
   * @returns {Promise<Array>} List of display objects
   */
  async _getDisplayList() {
    try {
      // Request display list via data channel
      const request = {
        type: 'get_display_list',
        timestamp: Date.now()
      };

      const success = this.webrtcConnection.sendData(JSON.stringify(request));
      if (!success) {
        throw new Error('Failed to request display list');
      }

      // In a real implementation, wait for response via data channel
      // For now, return empty array
      return [];

    } catch (error) {
      this.logger.error('Failed to get display list:', error);
      return [];
    }
  }

  /**
   * @brief Get current display information from server
   * @private
   * @returns {Promise<Object|null>} Current display object
   */
  async _getDisplayInfo() {
    try {
      // Request current display info via data channel
      const request = {
        type: 'get_display_info',
        timestamp: Date.now()
      };

      const success = this.webrtcConnection.sendData(JSON.stringify(request));
      if (!success) {
        throw new Error('Failed to request display info');
      }

      // In a real implementation, wait for response via data channel
      // For now, return null
      return null;

    } catch (error) {
      this.logger.error('Failed to get display info:', error);
      return null;
    }
  }

  /**
   * @brief Handle data channel message for display-related responses
   * @private
   * @param {string} message - Data channel message
   */
  _handleDataChannelMessage(message) {
    try {
      const data = JSON.parse(message);

      switch (data.type) {
        case 'display_list':
          // Update display selector with new list
          if (this.displaySelector && data.displays) {
            this.displaySelector.updateDisplayList(data.displays);
          }
          break;

        case 'display_info':
          // Update current display info
          if (this.displaySelector && data.display) {
            this.currentDisplayId = data.display.id;
            this.displaySelector.selectDisplay(data.display.id);
          }
          break;

        case 'display_switch_result':
          // Handle display switch result
          if (data.success) {
            this.logger.info('Display switch successful');
          } else {
            this.logger.error('Display switch failed:', data.error);
            // Revert selection
            if (this.displaySelector) {
              this.displaySelector.selectDisplay(this.currentDisplayId);
            }
          }
          break;

        default:
          // Ignore unknown message types
          break;
      }
    } catch (error) {
      this.logger.error('Failed to handle data channel message:', error);
    }
  }

  /**
   * @brief Switch to a specific display programmatically
   * @param {string} displayId - Display ID to switch to
   */
  async switchDisplay(displayId) {
    if (!this.displaySelector) {
      this.logger.warn('Display selector not enabled');
      return;
    }

    this.displaySelector.selectDisplay(displayId);
  }

  /**
   * @brief Get current display
   * @returns {Object|null} Current display object
   */
  getCurrentDisplay() {
    if (!this.displaySelector) {
      return null;
    }

    return this.displaySelector.getCurrentDisplay();
  }

  /**
   * @brief Enable/disable display selector
   * @param {boolean} enabled - Enable display selector
   */
  enableDisplaySelector(enabled) {
    if (this.displaySelector) {
      this.displaySelector.setEnabled(enabled);
    }
  }

  /**
   * @brief Handle error callback
   * @private
   */
  _onError(error) {
    if (this.onError) {
      this.onError(error);
    }
  }

  /**
   * @brief Destroy client and cleanup resources
   */
  destroy() {
    this.logger.info('Destroying client...');

    this.disconnect();

    if (this.displaySelector) {
      this.displaySelector.destroy();
      this.displaySelector = null;
    }

    if (this.videoRenderer) {
      this.videoRenderer.destroy();
      this.videoRenderer = null;
    }

    if (this.metricsDisplay) {
      this.metricsDisplay.destroy();
      this.metricsDisplay = null;
    }

    if (this.inputCapture) {
      this.inputCapture = null;
    }

    if (this.webrtcConnection) {
      this.webrtcConnection = null;
    }

    // Remove canvas from DOM
    const canvas = this.videoContainer.querySelector('.remote-desktop-canvas');
    if (canvas) {
      canvas.remove();
    }

    this.onConnected = null;
    this.onDisconnected = null;
    this.onError = null;

    this.logger.info('Client destroyed');
  }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = RemoteDesktopClient;
}
