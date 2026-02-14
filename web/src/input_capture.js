/**
 * @file input_capture.js
 * @brief Touch/mouse/keyboard input capture for remote desktop
 *
 * Captures user input events and converts to protocol messages:
 * - Mouse events (move, click, wheel)
 * - Touch events (tap, long-press - gestures handled separately)
 * - Keyboard events (keydown, keyup)
 * - Coordinate mapping (client to server screen coordinates)
 *
 * @module InputCapture
 */

'use strict';

/**
 * Input event types
 */
const InputEventType = {
  MOUSE_MOVE: 'mousemove',
  MOUSE_DOWN: 'mousedown',
  MOUSE_UP: 'mouseup',
  MOUSE_WHEEL: 'wheel',
  KEY_DOWN: 'keydown',
  KEY_UP: 'keyup',
  TOUCH_START: 'touchstart',
  TOUCH_MOVE: 'touchmove',
  TOUCH_END: 'touchend'
};

/**
 * Mouse button codes
 */
const MouseButton = {
  LEFT: 1,
  MIDDLE: 2,
  RIGHT: 3,
  BACK: 4,
  FORWARD: 5
};

/**
 * @classdesc Input capture handler
 * @class
 */
class InputCapture {
  /**
   * @constructor
   * @param {Object} config - Configuration
   * @param {HTMLElement} config.target - Target element to capture input from
   * @param {Function} config.onInput - Callback for input events
   * @param {Object} config.screenDims - Server screen dimensions {width, height}
   */
  constructor(config = {}) {
    this.target = config.target || window;
    this.onInput = config.onInput || null;
    this.screenDims = config.screenDims || { width: 1920, height: 1080 };

    this.enabled = false;
    this.captureKeyboard = true;
    this.captureMouse = true;
    this.captureTouch = true;

    this.mousePosition = { x: 0, y: 0 };
    this.mouseButtons = 0;
    this.keyModifiers = {
      shift: false,
      ctrl: false,
      alt: false,
      meta: false
    };

    this.longPressTimer = null;
    this.longPressThreshold = 500; // 500ms
    this.lastTouchStartTime = 0;
    this.lastTouchPosition = { x: 0, y: 0 };

    this.logger = this._createLogger();
  }

  /**
   * @brief Create console logger with prefix
   * @private
   */
  _createLogger() {
    return {
      info: (...args) => console.log('[InputCapture]', ...args),
      warn: (...args) => console.warn('[InputCapture]', ...args),
      error: (...args) => console.error('[InputCapture]', ...args)
    };
  }

  /**
   * @brief Enable input capture
   * @param {boolean} [keyboard=true] - Enable keyboard capture
   * @param {boolean} [mouse=true] - Enable mouse capture
   * @param {boolean} [touch=true] - Enable touch capture
   */
  enable(keyboard = true, mouse = true, touch = true) {
    if (this.enabled) {
      this.logger.warn('Input capture already enabled');
      return;
    }

    this.captureKeyboard = keyboard;
    this.captureMouse = mouse;
    this.captureTouch = touch;

    if (keyboard) {
      this._setupKeyboardListeners();
    }

    if (mouse) {
      this._setupMouseListeners();
    }

    if (touch) {
      this._setupTouchListeners();
    }

    this.enabled = true;
    this.logger.info('Input capture enabled');
  }

  /**
   * @brief Disable input capture
   */
  disable() {
    if (!this.enabled) {
      return;
    }

    this._removeKeyboardListeners();
    this._removeMouseListeners();
    this._removeTouchListeners();

    this.enabled = false;
    this.logger.info('Input capture disabled');
  }

  /**
   * @brief Setup keyboard event listeners
   * @private
   */
  _setupKeyboardListeners() {
    this.keydownHandler = (e) => this._onKeyDown(e);
    this.keyupHandler = (e) => this._onKeyUp(e);

    this.target.addEventListener('keydown', this.keydownHandler, { passive: false });
    this.target.addEventListener('keyup', this.keyupHandler, { passive: false });
  }

  /**
   * @brief Setup mouse event listeners
   * @private
   */
  _setupMouseListeners() {
    this.mousemoveHandler = (e) => this._onMouseMove(e);
    this.mousedownHandler = (e) => this._onMouseDown(e);
    this.mouseupHandler = (e) => this._onMouseUp(e);
    this.wheelHandler = (e) => this._onWheel(e);

    this.target.addEventListener('mousemove', this.mousemoveHandler);
    this.target.addEventListener('mousedown', this.mousedownHandler);
    this.target.addEventListener('mouseup', this.mouseupHandler);
    this.target.addEventListener('wheel', this.wheelHandler, { passive: false });

    // Prevent context menu
    this.target.addEventListener('contextmenu', (e) => e.preventDefault());
  }

  /**
   * @brief Setup touch event listeners
   * @private
   */
  _setupTouchListeners() {
    this.touchstartHandler = (e) => this._onTouchStart(e);
    this.touchmoveHandler = (e) => this._onTouchMove(e);
    this.touchendHandler = (e) => this._onTouchEnd(e);

    this.target.addEventListener('touchstart', this.touchstartHandler, { passive: false });
    this.target.addEventListener('touchmove', this.touchmoveHandler, { passive: false });
    this.target.addEventListener('touchend', this.touchendHandler, { passive: false });
  }

  /**
   * @brief Remove keyboard event listeners
   * @private
   */
  _removeKeyboardListeners() {
    if (this.keydownHandler) {
      this.target.removeEventListener('keydown', this.keydownHandler);
      this.keydownHandler = null;
    }
    if (this.keyupHandler) {
      this.target.removeEventListener('keyup', this.keyupHandler);
      this.keyupHandler = null;
    }
  }

  /**
   * @brief Remove mouse event listeners
   * @private
   */
  _removeMouseListeners() {
    if (this.mousemoveHandler) {
      this.target.removeEventListener('mousemove', this.mousemoveHandler);
      this.mousemoveHandler = null;
    }
    if (this.mousedownHandler) {
      this.target.removeEventListener('mousedown', this.mousedownHandler);
      this.mousedownHandler = null;
    }
    if (this.mouseupHandler) {
      this.target.removeEventListener('mouseup', this.mouseupHandler);
      this.mouseupHandler = null;
    }
    if (this.wheelHandler) {
      this.target.removeEventListener('wheel', this.wheelHandler);
      this.wheelHandler = null;
    }
  }

  /**
   * @brief Remove touch event listeners
   * @private
   */
  _removeTouchListeners() {
    if (this.touchstartHandler) {
      this.target.removeEventListener('touchstart', this.touchstartHandler);
      this.touchstartHandler = null;
    }
    if (this.touchmoveHandler) {
      this.target.removeEventListener('touchmove', this.touchmoveHandler);
      this.touchmoveHandler = null;
    }
    if (this.touchendHandler) {
      this.target.removeEventListener('touchend', this.touchendHandler);
      this.touchendHandler = null;
    }
  }

  /**
   * @brief Handle keydown event
   * @private
   */
  _onKeyDown(e) {
    if (!this.captureKeyboard) return;

    // Update modifier state
    this._updateModifiers(e);

    const event = {
      type: InputEventType.KEY_DOWN,
      keyCode: e.keyCode,
      key: e.key,
      code: e.code,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    // Prevent default for non-input keys
    if (!this._isInputKey(e.code)) {
      e.preventDefault();
    }
  }

  /**
   * @brief Handle keyup event
   * @private
   */
  _onKeyUp(e) {
    if (!this.captureKeyboard) return;

    // Update modifier state
    this._updateModifiers(e);

    const event = {
      type: InputEventType.KEY_UP,
      keyCode: e.keyCode,
      key: e.key,
      code: e.code,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    if (!this._isInputKey(e.code)) {
      e.preventDefault();
    }
  }

  /**
   * @brief Handle mousemove event
   * @private
   */
  _onMouseMove(e) {
    if (!this.captureMouse) return;

    const clientPos = this._getClientPosition(e);
    const serverPos = this._mapToServer(clientPos);

    this.mousePosition = serverPos;

    const event = {
      type: InputEventType.MOUSE_MOVE,
      x: serverPos.x,
      y: serverPos.y,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);
  }

  /**
   * @brief Handle mousedown event
   * @private
   */
  _onMouseDown(e) {
    if (!this.captureMouse) return;

    const button = this._getMouseButton(e);
    this.mouseButtons |= button;

    const clientPos = this._getClientPosition(e);
    const serverPos = this._mapToServer(clientPos);

    const event = {
      type: InputEventType.MOUSE_DOWN,
      x: serverPos.x,
      y: serverPos.y,
      button: button,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    e.preventDefault();
  }

  /**
   * @brief Handle mouseup event
   * @private
   */
  _onMouseUp(e) {
    if (!this.captureMouse) return;

    const button = this._getMouseButton(e);
    this.mouseButtons &= ~button;

    const clientPos = this._getClientPosition(e);
    const serverPos = this._mapToServer(clientPos);

    const event = {
      type: InputEventType.MOUSE_UP,
      x: serverPos.x,
      y: serverPos.y,
      button: button,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    e.preventDefault();
  }

  /**
   * @brief Handle wheel event
   * @private
   */
  _onWheel(e) {
    if (!this.captureMouse) return;

    const clientPos = this._getClientPosition(e);
    const serverPos = this._mapToServer(clientPos);

    const event = {
      type: InputEventType.MOUSE_WHEEL,
      x: serverPos.x,
      y: serverPos.y,
      deltaX: e.deltaX,
      deltaY: e.deltaY,
      deltaZ: e.deltaZ,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    e.preventDefault();
  }

  /**
   * @brief Handle touchstart event
   * @private
   */
  _onTouchStart(e) {
    if (!this.captureTouch) return;

    const touch = e.touches[0];
    this.lastTouchStartTime = Date.now();
    this.lastTouchPosition = this._getClientPosition(touch);

    // Start long-press timer
    this.longPressTimer = setTimeout(() => {
      this._onLongPress(touch);
    }, this.longPressThreshold);

    e.preventDefault();
  }

  /**
   * @brief Handle touchmove event
   * @private
   */
  _onTouchMove(e) {
    if (!this.captureTouch) return;

    // Cancel long-press on movement
    if (this.longPressTimer) {
      clearTimeout(this.longPressTimer);
      this.longPressTimer = null;
    }

    const touch = e.touches[0];
    const clientPos = this._getClientPosition(touch);
    const serverPos = this._mapToServer(clientPos);

    // Send as mouse move
    const event = {
      type: InputEventType.MOUSE_MOVE,
      x: serverPos.x,
      y: serverPos.y,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    e.preventDefault();
  }

  /**
   * @brief Handle touchend event
   * @private
   */
  _onTouchEnd(e) {
    if (!this.captureTouch) return;

    // Cancel long-press timer
    if (this.longPressTimer) {
      clearTimeout(this.longPressTimer);
      this.longPressTimer = null;
    }

    const touch = e.changedTouches[0];
    const clientPos = this._getClientPosition(touch);
    const serverPos = this._mapToServer(clientPos);

    const pressDuration = Date.now() - this.lastTouchStartTime;

    // Send mouse click
    const event = {
      type: InputEventType.MOUSE_DOWN,
      x: serverPos.x,
      y: serverPos.y,
      button: MouseButton.LEFT,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    // Send mouse up immediately
    setTimeout(() => {
      this._sendInput({
        type: InputEventType.MOUSE_UP,
        x: serverPos.x,
        y: serverPos.y,
        button: MouseButton.LEFT,
        modifiers: { ...this.keyModifiers }
      });
    }, 50);

    e.preventDefault();
  }

  /**
   * @brief Handle long-press (right-click)
   * @private
   */
  _onLongPress(touch) {
    this.longPressTimer = null;

    const clientPos = this._getClientPosition(touch);
    const serverPos = this._mapToServer(clientPos);

    // Send right-click
    const event = {
      type: InputEventType.MOUSE_DOWN,
      x: serverPos.x,
      y: serverPos.y,
      button: MouseButton.RIGHT,
      modifiers: { ...this.keyModifiers }
    };

    this._sendInput(event);

    // Send mouse up after short delay
    setTimeout(() => {
      this._sendInput({
        type: InputEventType.MOUSE_UP,
        x: serverPos.x,
        y: serverPos.y,
        button: MouseButton.RIGHT,
        modifiers: { ...this.keyModifiers }
      });
    }, 50);

    this.logger.info('Long-press detected, sent right-click');
  }

  /**
   * @brief Get client position from event
   * @private
   */
  _getClientPosition(e) {
    const rect = this.target.getBoundingClientRect();
    return {
      x: e.clientX - rect.left,
      y: e.clientY - rect.top
    };
  }

  /**
   * @brief Map client position to server screen coordinates
   * @private
   */
  _mapToServer(clientPos) {
    const rect = this.target.getBoundingClientRect();
    const scaleX = this.screenDims.width / rect.width;
    const scaleY = this.screenDims.height / rect.height;

    return {
      x: Math.floor(clientPos.x * scaleX),
      y: Math.floor(clientPos.y * scaleY)
    };
  }

  /**
   * @brief Get mouse button code
   * @private
   */
  _getMouseButton(e) {
    switch (e.button) {
      case 0: return MouseButton.LEFT;
      case 1: return MouseButton.MIDDLE;
      case 2: return MouseButton.RIGHT;
      case 3: return MouseButton.BACK;
      case 4: return MouseButton.FORWARD;
      default: return 0;
    }
  }

  /**
   * @brief Update modifier key state
   * @private
   */
  _updateModifiers(e) {
    this.keyModifiers.shift = e.shiftKey;
    this.keyModifiers.ctrl = e.ctrlKey;
    this.keyModifiers.alt = e.altKey;
    this.keyModifiers.meta = e.metaKey;
  }

  /**
   * @brief Check if key is an input key (allow default behavior)
   * @private
   */
  _isInputKey(code) {
    // Allow default for these keys
    return code === 'Tab' || code.startsWith('F') ||
           code === 'CapsLock' || code === 'NumLock' ||
           code === 'ScrollLock';
  }

  /**
   * @brief Send input event to callback
   * @private
   */
  _sendInput(event) {
    if (this.onInput) {
      this.onInput(event);
    }
  }

  /**
   * @brief Set server screen dimensions
   * @param {number} width - Screen width
   * @param {number} height - Screen height
   */
  setScreenDimensions(width, height) {
    this.screenDims = { width, height };
    this.logger.info('Screen dimensions set:', width, 'x', height);
  }

  /**
   * @brief Check if input capture is enabled
   * @returns {boolean} Enabled status
   */
  isEnabled() {
    return this.enabled;
  }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = InputCapture;
  module.exports.InputEventType = InputEventType;
  module.exports.MouseButton = MouseButton;
}
