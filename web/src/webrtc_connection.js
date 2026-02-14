/**
 * @file webrtc_connection.js
 * @brief WebRTC peer connection management for remote desktop
 *
 * Handles WebRTC peer connection establishment:
 * - SDP offer/answer exchange via signaling server
 * - ICE candidate exchange
 * - Video track receiving
 * - Data channel for input transmission
 * - Connection state management
 *
 * @module WebrtcConnection
 */

'use strict';

/**
 * @classdesc WebRTC peer connection for remote desktop
 * @class
 */
class WebrtcConnection {
  /**
   * @constructor
   * @param {Object} config - Connection configuration
   * @param {string} config.signalingUrl - WebSocket signaling server URL
   * @param {Object} config.rtcConfig - WebRTC configuration (ICE servers, etc.)
   */
  constructor(config = {}) {
    this.signalingUrl = config.signalingUrl || 'ws://localhost:8080';
    this.rtcConfig = config.rtcConfig || {
      iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
    };

    this.pc = null;
    this.signalingSocket = null;
    this.videoTrack = null;
    this.dataChannel = null;

    this.onTrack = null;
    this.onDataChannelOpen = null;
    this.onDataChannelMessage = null;
    this.onDataChannelClose = null;
    this.onIceCandidate = null;
    this.onConnectionStateChange = null;
    this.onError = null;

    this.connected = false;
    this.iceCandidates = [];

    this.logger = this._createLogger();
  }

  /**
   * @brief Create console logger with prefix
   * @private
   */
  _createLogger() {
    return {
      info: (...args) => console.log('[WebrtcConnection]', ...args),
      warn: (...args) => console.warn('[WebrtcConnection]', ...args),
      error: (...args) => console.error('[WebrtcConnection]', ...args)
    };
  }

  /**
   * @brief Initialize WebRTC peer connection
   * @returns {Promise<void>}
   */
  async initialize() {
    try {
      this.logger.info('Initializing WebRTC connection...');

      // Create RTCPeerConnection
      this.pc = new RTCPeerConnection(this.rtcConfig);

      this._setupPeerConnectionListeners();

      this.logger.info('WebRTC connection initialized');
    } catch (error) {
      this.logger.error('Failed to initialize:', error);
      this._onError(error);
      throw error;
    }
  }

  /**
   * @brief Setup peer connection event listeners
   * @private
   */
  _setupPeerConnectionListeners() {
    // ICE candidate handler
    this.pc.onicecandidate = (event) => {
      if (event.candidate) {
        this.iceCandidates.push(event.candidate);
        this.logger.info('ICE candidate generated:', event.candidate.candidate);

        if (this.onIceCandidate) {
          this.onIceCandidate(event.candidate);
        }
      } else {
        this.logger.info('ICE gathering complete');
      }
    };

    // Track handler (video stream)
    this.pc.ontrack = (event) => {
      this.logger.info('Received track:', event.track.kind);

      if (event.track.kind === 'video') {
        this.videoTrack = event.track;

        if (this.onTrack) {
          this.onTrack(event.streams[0]);
        }
      }
    };

    // Data channel handler (server-initiated)
    this.pc.ondatachannel = (event) => {
      this.logger.info('Data channel received:', event.channel.label);
      this.dataChannel = event.channel;
      this._setupDataChannelListeners();
    };

    // Connection state handler
    this.pc.onconnectionstatechange = () => {
      const state = this.pc.connectionState;
      this.logger.info('Connection state:', state);

      this.connected = (state === 'connected');

      if (this.onConnectionStateChange) {
        this.onConnectionStateChange(state);
      }
    };

    // ICE connection state handler
    this.pc.oniceconnectionstatechange = () => {
      const state = this.pc.iceConnectionState;
      this.logger.info('ICE connection state:', state);

      if (state === 'failed' || state === 'disconnected') {
        this.logger.warn('ICE connection failed or disconnected');
        this._onError(new Error(`ICE connection ${state}`));
      }
    };
  }

  /**
   * @brief Setup data channel event listeners
   * @private
   */
  _setupDataChannelListeners() {
    this.dataChannel.onopen = () => {
      this.logger.info('Data channel opened');
      if (this.onDataChannelOpen) {
        this.onDataChannelOpen();
      }
    };

    this.dataChannel.onmessage = (event) => {
      if (this.onDataChannelMessage) {
        this.onDataChannelMessage(event.data);
      }
    };

    this.dataChannel.onclose = () => {
      this.logger.info('Data channel closed');
      if (this.onDataChannelClose) {
        this.onDataChannelClose();
      }
    };

    this.dataChannel.onerror = (error) => {
      this.logger.error('Data channel error:', error);
      this._onError(error);
    };
  }

  /**
   * @brief Create SDP offer (as connecting client)
   * @returns {Promise<string>} SDP offer
   */
  async createOffer() {
    if (!this.pc) {
      throw new Error('Peer connection not initialized');
    }

    this.logger.info('Creating offer...');

    const offer = await this.pc.createOffer({
      offerToReceiveAudio: false,
      offerToReceiveVideo: true
    });

    await this.pc.setLocalDescription(offer);

    this.logger.info('Offer created:', offer.type);

    return offer.sdp;
  }

  /**
   * @brief Create SDP answer (as answering server)
   * @returns {Promise<string>} SDP answer
   */
  async createAnswer() {
    if (!this.pc) {
      throw new Error('Peer connection not initialized');
    }

    this.logger.info('Creating answer...');

    const answer = await this.pc.createAnswer();
    await this.pc.setLocalDescription(answer);

    this.logger.info('Answer created:', answer.type);

    return answer.sdp;
  }

  /**
   * @brief Set remote SDP description
   * @param {string} sdp - Remote SDP string
   * @param {string} type - SDP type ('offer' or 'answer')
   */
  async setRemoteDescription(sdp, type) {
    if (!this.pc) {
      throw new Error('Peer connection not initialized');
    }

    this.logger.info('Setting remote description:', type);

    const description = new RTCSessionDescription({
      type: type,
      sdp: sdp
    });

    await this.pc.setRemoteDescription(description);

    this.logger.info('Remote description set');
  }

  /**
   * @brief Add ICE candidate to peer connection
   * @param {RTCIceCandidate} candidate - ICE candidate
   */
  async addIceCandidate(candidate) {
    if (!this.pc) {
      throw new Error('Peer connection not initialized');
    }

    try {
      await this.pc.addIceCandidate(candidate);
      this.logger.info('ICE candidate added');
    } catch (error) {
      this.logger.warn('Failed to add ICE candidate:', error);
    }
  }

  /**
   * @brief Send data via data channel
   * @param {string|ArrayBuffer|Blob} data - Data to send
   * @returns {boolean} Success
   */
  sendData(data) {
    if (!this.dataChannel || this.dataChannel.readyState !== 'open') {
      this.logger.warn('Data channel not ready');
      return false;
    }

    try {
      this.dataChannel.send(data);
      return true;
    } catch (error) {
      this.logger.error('Failed to send data:', error);
      return false;
    }
  }

  /**
   * @brief Close connection
   */
  close() {
    this.logger.info('Closing connection...');

    if (this.dataChannel) {
      this.dataChannel.close();
      this.dataChannel = null;
    }

    if (this.pc) {
      this.pc.close();
      this.pc = null;
    }

    this.videoTrack = null;
    this.iceCandidates = [];
    this.connected = false;

    this.logger.info('Connection closed');
  }

  /**
   * @brief Get connection state
   * @returns {string} Connection state
   */
  getConnectionState() {
    return this.pc ? this.pc.connectionState : 'new';
  }

  /**
   * @brief Check if connected
   * @returns {boolean} Connected status
   */
  isConnected() {
    return this.connected;
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
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = WebrtcConnection;
}
