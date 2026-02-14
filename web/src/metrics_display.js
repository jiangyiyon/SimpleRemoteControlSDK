/**
 * @file metrics_display.js
 * @brief Latency measurement and metrics display for remote desktop
 *
 * Tracks and displays performance metrics:
 * - Input-to-display latency
 * - Network round-trip time (RTT)
 * - Frame rate (FPS)
 * - Data channel throughput
 * - Connection state
 *
 * @module MetricsDisplay
 */

'use strict';

/**
 * @classdesc Metrics display and measurement
 * @class
 */
class MetricsDisplay {
  /**
   * @constructor
   * @param {Object} config - Configuration
   * @param {HTMLElement} config.container - Container element for metrics display
   * @param {number} config.updateInterval - Update interval in ms (default: 1000)
   */
  constructor(config = {}) {
    this.container = config.container || document.body;
    this.updateInterval = config.updateInterval || 1000;

    this.latencySamples = [];
    this.rttSamples = [];
    this.fps = 0;
    this.throughput = 0;

    this.lastInputTimestamp = 0;
    this.lastFrameTimestamp = 0;
    this.lastRttTimestamp = 0;
    this.bytesSent = 0;
    this.bytesReceived = 0;

    this.maxSamples = 100; // Keep last 100 samples
    this.enabled = false;
    this.updateTimer = null;

    this.logger = this._createLogger();
    this._createDisplay();
  }

  /**
   * @brief Create console logger with prefix
   * @private
   */
  _createLogger() {
    return {
      info: (...args) => console.log('[MetricsDisplay]', ...args),
      warn: (...args) => console.warn('[MetricsDisplay]', ...args),
      error: (...args) => console.error('[MetricsDisplay]', ...args)
    };
  }

  /**
   * @brief Create metrics display UI
   * @private
   */
  _createDisplay() {
    this.display = document.createElement('div');
    this.display.className = 'metrics-display';
    this.display.style.cssText = `
      position: fixed;
      top: 10px;
      right: 10px;
      background: rgba(0, 0, 0, 0.7);
      color: #00ff00;
      font-family: monospace;
      font-size: 12px;
      padding: 10px;
      border-radius: 5px;
      z-index: 1000;
      display: none;
    `;

    this.display.innerHTML = `
      <div class="metrics-header">Remote Desktop Metrics</div>
      <div class="metrics-item">
        <span class="metrics-label">Latency:</span>
        <span class="metrics-value" id="metrics-latency">--</span> ms
      </div>
      <div class="metrics-item">
        <span class="metrics-label">RTT:</span>
        <span class="metrics-value" id="metrics-rtt">--</span> ms
      </div>
      <div class="metrics-item">
        <span class="metrics-label">FPS:</span>
        <span class="metrics-value" id="metrics-fps">--</span>
      </div>
      <div class="metrics-item">
        <span class="metrics-label">Throughput:</span>
        <span class="metrics-value" id="metrics-throughput">--</span> KB/s
      </div>
      <div class="metrics-item">
        <span class="metrics-label">State:</span>
        <span class="metrics-value" id="metrics-state">--</span>
      </div>
    `;

    this.container.appendChild(this.display);

    // Cache element references
    this.latencyElement = this.display.querySelector('#metrics-latency');
    this.rttElement = this.display.querySelector('#metrics-rtt');
    this.fpsElement = this.display.querySelector('#metrics-fps');
    this.throughputElement = this.display.querySelector('#metrics-throughput');
    this.stateElement = this.display.querySelector('#metrics-state');
  }

  /**
   * @brief Enable metrics display
   * @param {boolean} [show=true] - Show display
   */
  enable(show = true) {
    if (this.enabled) {
      this.logger.warn('Metrics display already enabled');
      return;
    }

    this.enabled = true;
    this.display.style.display = show ? 'block' : 'none';

    // Start update timer
    this.updateTimer = setInterval(() => this._updateDisplay(), this.updateInterval);

    this.logger.info('Metrics display enabled');
  }

  /**
   * @brief Disable metrics display
   */
  disable() {
    if (!this.enabled) {
      return;
    }

    this.enabled = false;
    this.display.style.display = 'none';

    if (this.updateTimer) {
      clearInterval(this.updateTimer);
      this.updateTimer = null;
    }

    this.logger.info('Metrics display disabled');
  }

  /**
   * @brief Show/hide metrics display
   * @param {boolean} show - Show display
   */
  show(show) {
    if (this.enabled) {
      this.display.style.display = show ? 'block' : 'none';
    }
  }

  /**
   * @brief Record input event timestamp
   * @param {number} [timestamp] - Timestamp (default: performance.now())
   */
  recordInput(timestamp = performance.now()) {
    this.lastInputTimestamp = timestamp;
  }

  /**
   * @brief Record frame render timestamp
   * @param {number} [timestamp] - Timestamp (default: performance.now())
   */
  recordFrame(timestamp = performance.now()) {
    this.lastFrameTimestamp = timestamp;

    // Calculate latency if we have input timestamp
    if (this.lastInputTimestamp > 0) {
      const latency = this.lastFrameTimestamp - this.lastInputTimestamp;
      this._addLatencySample(latency);
      this.lastInputTimestamp = 0; // Reset for next cycle
    }
  }

  /**
   * @brief Record RTT measurement
   * @param {number} rtt - Round-trip time in ms
   */
  recordRtt(rtt) {
    this._addRttSample(rtt);
    this.lastRttTimestamp = performance.now();
  }

  /**
   * @brief Update FPS
   * @param {number} fps - Frames per second
   */
  updateFps(fps) {
    this.fps = fps;
  }

  /**
   * @brief Record data channel throughput
   * @param {number} bytes - Bytes sent/received
   * @param {string} [direction='sent'] - 'sent' or 'received'
   */
  recordData(bytes, direction = 'sent') {
    if (direction === 'sent') {
      this.bytesSent += bytes;
    } else {
      this.bytesReceived += bytes;
    }
  }

  /**
   * @brief Set connection state
   * @param {string} state - Connection state
   */
  setState(state) {
    this.stateElement.textContent = state;

    // Color-code states
    if (state === 'connected') {
      this.stateElement.style.color = '#00ff00';
    } else if (state === 'connecting') {
      this.stateElement.style.color = '#ffff00';
    } else {
      this.stateElement.style.color = '#ff0000';
    }
  }

  /**
   * @brief Add latency sample
   * @private
   */
  _addLatencySample(latency) {
    this.latencySamples.push(latency);
    if (this.latencySamples.length > this.maxSamples) {
      this.latencySamples.shift();
    }
  }

  /**
   * @brief Add RTT sample
   * @private
   */
  _addRttSample(rtt) {
    this.rttSamples.push(rtt);
    if (this.rttSamples.length > this.maxSamples) {
      this.rttSamples.shift();
    }
  }

  /**
   * @brief Calculate average from samples
   * @private
   */
  _calculateAverage(samples) {
    if (samples.length === 0) return 0;

    const sum = samples.reduce((acc, val) => acc + val, 0);
    return sum / samples.length;
  }

  /**
   * @brief Calculate percentile from samples
   * @private
   */
  _calculatePercentile(samples, percentile) {
    if (samples.length === 0) return 0;

    const sorted = [...samples].sort((a, b) => a - b);
    const index = Math.ceil((percentile / 100) * sorted.length) - 1;
    return sorted[index];
  }

  /**
   * @brief Update display with current metrics
   * @private
   */
  _updateDisplay() {
    // Calculate latency statistics
    const avgLatency = this._calculateAverage(this.latencySamples);
    const p95Latency = this._calculatePercentile(this.latencySamples, 95);
    const p99Latency = this._calculatePercentile(this.latencySamples, 99);

    // Calculate RTT statistics
    const avgRtt = this._calculateAverage(this.rttSamples);
    const p95Rtt = this._calculatePercentile(this.rttSamples, 95);

    // Calculate throughput
    const timeSinceUpdate = this.updateInterval / 1000; // Convert to seconds
    const totalBytes = this.bytesSent + this.bytesReceived;
    const throughput = (totalBytes / timeSinceUpdate) / 1024; // KB/s

    // Reset counters
    this.bytesSent = 0;
    this.bytesReceived = 0;

    // Update display
    this.latencyElement.textContent = this._formatLatency(avgLatency, p95Latency);
    this.rttElement.textContent = avgRtt > 0 ? avgRtt.toFixed(1) : '--';
    this.fpsElement.textContent = this.fps > 0 ? this.fps.toFixed(1) : '--';
    this.throughputElement.textContent = throughput > 0 ? throughput.toFixed(1) : '--';

    // Color-code latency
    if (avgLatency > 100) {
      this.latencyElement.style.color = '#ff0000';
    } else if (avgLatency > 50) {
      this.latencyElement.style.color = '#ffff00';
    } else {
      this.latencyElement.style.color = '#00ff00';
    }
  }

  /**
   * @brief Format latency display
   * @private
   */
  _formatLatency(avg, p95) {
    if (avg === 0) return '--';
    return `${avg.toFixed(1)} (P95: ${p95.toFixed(1)})`;
  }

  /**
   * @brief Get current latency statistics
   * @returns {Object} {avg, p95, p99}
   */
  getLatencyStats() {
    return {
      avg: this._calculateAverage(this.latencySamples),
      p95: this._calculatePercentile(this.latencySamples, 95),
      p99: this._calculatePercentile(this.latencySamples, 99)
    };
  }

  /**
   * @brief Get current RTT statistics
   * @returns {Object} {avg, p95}
   */
  getRttStats() {
    return {
      avg: this._calculateAverage(this.rttSamples),
      p95: this._calculatePercentile(this.rttSamples, 95)
    };
  }

  /**
   * @brief Clear all samples
   */
  clearSamples() {
    this.latencySamples = [];
    this.rttSamples = [];
    this.logger.info('Samples cleared');
  }

  /**
   * @brief Destroy metrics display
   */
  destroy() {
    this.disable();
    if (this.display && this.display.parentNode) {
      this.display.parentNode.removeChild(this.display);
    }
    this.logger.info('Metrics display destroyed');
  }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = MetricsDisplay;
}
