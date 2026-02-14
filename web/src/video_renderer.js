/**
 * @file video_renderer.js
 * @brief Canvas 2D video renderer for remote desktop
 *
 * Handles video stream rendering to HTML5 canvas:
 * - MediaStream to canvas rendering
 * - Frame rate monitoring
 * - Canvas resize handling
 * - Aspect ratio preservation
 * - Performance optimization (requestAnimationFrame)
 *
 * @module VideoRenderer
 */

'use strict';

/**
 * @classdesc Canvas 2D video renderer
 * @class
 */
class VideoRenderer {
  /**
   * @constructor
   * @param {Object} config - Renderer configuration
   * @param {HTMLCanvasElement} config.canvas - Canvas element to render to
   * @param {number} config.targetFps - Target frame rate (default: 60)
   * @param {boolean} config.smoothScaling - Enable smooth scaling (default: false)
   */
  constructor(config = {}) {
    this.canvas = config.canvas || document.createElement('canvas');
    this.ctx = this.canvas.getContext('2d', { alpha: false });
    this.targetFps = config.targetFps || 60;
    this.smoothScaling = config.smoothScaling !== undefined ? config.smoothScaling : false;

    this.videoElement = document.createElement('video');
    this.videoElement.autoplay = true;
    this.videoElement.muted = true;
    this.videoElement.playsInline = true;

    this.mediaStream = null;
    this.animationFrameId = null;
    this.lastFrameTime = 0;
    this.frameCount = 0;
    this.fps = 0;
    this.fpsUpdateInterval = 1000; // Update FPS every 1 second
    this.lastFpsUpdate = 0;

    this.onFrameRendered = null;
    this.onFpsUpdate = null;

    this.rendering = false;

    this.logger = this._createLogger();
    this._setupCanvas();
  }

  /**
   * @brief Create console logger with prefix
   * @private
   */
  _createLogger() {
    return {
      info: (...args) => console.log('[VideoRenderer]', ...args),
      warn: (...args) => console.warn('[VideoRenderer]', ...args),
      error: (...args) => console.error('[VideoRenderer]', ...args)
    };
  }

  /**
   * @brief Setup canvas properties
   * @private
   */
  _setupCanvas() {
    this.ctx.imageSmoothingEnabled = this.smoothScaling;
    this.ctx.imageSmoothingQuality = 'low';
  }

  /**
   * @brief Set video stream to render
   * @param {MediaStream} stream - MediaStream to render
   */
  setStream(stream) {
    this.logger.info('Setting video stream...');

    if (this.mediaStream) {
      this.stop();
    }

    this.mediaStream = stream;
    this.videoElement.srcObject = stream;

    this.videoElement.onloadedmetadata = () => {
      this.logger.info('Video metadata loaded:',
                      this.videoElement.videoWidth, 'x', this.videoElement.videoHeight);
      this.resizeCanvas();
    };

    this.videoElement.onloadeddata = () => {
      this.logger.info('Video data loaded');
      this.start();
    };
  }

  /**
   * @brief Start rendering loop
   */
  start() {
    if (this.rendering) {
      this.logger.warn('Already rendering');
      return;
    }

    this.logger.info('Starting render loop...');
    this.rendering = true;
    this.lastFrameTime = performance.now();
    this.lastFpsUpdate = performance.now();
    this.frameCount = 0;

    this._renderLoop();
  }

  /**
   * @brief Stop rendering loop
   */
  stop() {
    this.logger.info('Stopping render loop...');

    this.rendering = false;

    if (this.animationFrameId !== null) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }
  }

  /**
   * @brief Render loop using requestAnimationFrame
   * @private
   */
  _renderLoop() {
    if (!this.rendering) {
      return;
    }

    const now = performance.now();
    const frameInterval = 1000 / this.targetFps;
    const deltaTime = now - this.lastFrameTime;

    if (deltaTime >= frameInterval) {
      this._renderFrame();
      this.lastFrameTime = now - (deltaTime % frameInterval);
    }

    // Update FPS counter
    if (now - this.lastFpsUpdate >= this.fpsUpdateInterval) {
      this.fps = this.frameCount * 1000 / (now - this.lastFpsUpdate);
      this.frameCount = 0;
      this.lastFpsUpdate = now;

      if (this.onFpsUpdate) {
        this.onFpsUpdate(this.fps);
      }
    }

    this.animationFrameId = requestAnimationFrame(() => this._renderLoop());
  }

  /**
   * @brief Render single frame to canvas
   * @private
   */
  _renderFrame() {
    if (this.videoElement.readyState < 2) {
      return;
    }

    const vw = this.videoElement.videoWidth;
    const vh = this.videoElement.videoHeight;

    if (vw === 0 || vh === 0) {
      return;
    }

    // Clear canvas
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    // Draw video frame
    this.ctx.drawImage(this.videoElement, 0, 0, this.canvas.width, this.canvas.height);

    this.frameCount++;

    if (this.onFrameRendered) {
      this.onFrameRendered();
    }
  }

  /**
   * @brief Resize canvas to match video aspect ratio
   * @param {number} [maxWidth] - Maximum width (default: container width)
   * @param {number} [maxHeight] - Maximum height (default: container height)
   */
  resizeCanvas(maxWidth, maxHeight) {
    if (!this.mediaStream) {
      return;
    }

    const vw = this.videoElement.videoWidth;
    const vh = this.videoElement.videoHeight;

    if (vw === 0 || vh === 0) {
      return;
    }

    // Get container dimensions if not specified
    if (!maxWidth) {
      maxWidth = this.canvas.parentElement.clientWidth || window.innerWidth;
    }
    if (!maxHeight) {
      maxHeight = this.canvas.parentElement.clientHeight || window.innerHeight;
    }

    // Calculate scaled dimensions preserving aspect ratio
    const scale = Math.min(maxWidth / vw, maxHeight / vh);
    const width = Math.floor(vw * scale);
    const height = Math.floor(vh * scale);

    this.canvas.width = width;
    this.canvas.height = height;

    this.logger.info('Canvas resized:', width, 'x', height);
  }

  /**
   * @brief Set canvas size explicitly
   * @param {number} width - Canvas width
   * @param {number} height - Canvas height
   */
  setCanvasSize(width, height) {
    this.canvas.width = width;
    this.canvas.height = height;
    this.logger.info('Canvas size set:', width, 'x', height);
  }

  /**
   * @brief Get current FPS
   * @returns {number} Current FPS
   */
  getFps() {
    return this.fps;
  }

  /**
   * @brief Get canvas dimensions
   * @returns {Object} {width, height}
   */
  getDimensions() {
    return {
      width: this.canvas.width,
      height: this.canvas.height
    };
  }

  /**
   * @brief Get video dimensions
   * @returns {Object} {width, height}
   */
  getVideoDimensions() {
    return {
      width: this.videoElement.videoWidth,
      height: this.videoElement.videoHeight
    };
  }

  /**
   * @brief Clear canvas
   */
  clear() {
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
  }

  /**
   * @brief Get canvas context
   * @returns {CanvasRenderingContext2D} Canvas context
   */
  getContext() {
    return this.ctx;
  }

  /**
   * @brief Get canvas element
   * @returns {HTMLCanvasElement} Canvas element
   */
  getCanvas() {
    return this.canvas;
  }

  /**
   * @brief Destroy renderer and cleanup resources
   */
  destroy() {
    this.logger.info('Destroying renderer...');

    this.stop();

    if (this.mediaStream) {
      this.mediaStream.getTracks().forEach(track => track.stop());
      this.mediaStream = null;
    }

    this.videoElement.srcObject = null;
    this.videoElement.remove();

    this.onFrameRendered = null;
    this.onFpsUpdate = null;
  }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = VideoRenderer;
}
