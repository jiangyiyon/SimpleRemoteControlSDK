/**
 * Display Selector UI Component
 * 
 * Provides a dropdown menu for selecting and switching between available displays.
 * Handles display list retrieval, selection changes, and SDP renegotiation for
 * seamless display switching.
 */

export class DisplaySelector {
  /**
   * @param {HTMLElement} container - Container element for the selector
   * @param {Object} options - Configuration options
   * @param {Function} options.onDisplaySelect - Callback when display is selected
   * @param {Function} options.getDisplayList - Function to fetch available displays
   * @param {Function} options.getDisplayInfo - Function to get current display info
   */
  constructor(container, options = {}) {
    this.container = container;
    this.onDisplaySelect = options.onDisplaySelect || (() => {});
    this.getDisplayList = options.getDisplayList || (() => Promise.resolve([]));
    this.getDisplayInfo = options.getDisplayInfo || (() => Promise.resolve(null));
    
    this.displays = [];
    this.currentDisplayId = null;
    this.isRefreshing = false;
    
    this._createUI();
    this._loadDisplayList();
  }

  /**
   * Create the selector UI elements
   * @private
   */
  _createUI() {
    this.container.innerHTML = `
      <div class="display-selector">
        <label for="display-select" class="display-selector-label">Display</label>
        <div class="display-select-wrapper">
          <select id="display-select" class="display-select" disabled>
            <option value="">Loading displays...</option>
          </select>
          <button id="display-refresh-btn" class="display-refresh-btn" title="Refresh displays">
            <svg class="refresh-icon" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M23 4v6h-6M1 20v-6h6"></path>
              <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path>
            </svg>
          </button>
        </div>
        <div id="display-info" class="display-info hidden">
          <div class="display-info-item">
            <span class="display-info-label">Resolution:</span>
            <span id="display-resolution" class="display-info-value">-</span>
          </div>
          <div class="display-info-item">
            <span class="display-info-label">Position:</span>
            <span id="display-position" class="display-info-value">-</span>
          </div>
        </div>
      </div>
    `;

    this.selectElement = this.container.querySelector('#display-select');
    this.refreshButton = this.container.querySelector('#display-refresh-btn');
    this.displayInfo = this.container.querySelector('#display-info');
    this.resolutionElement = this.container.querySelector('#display-resolution');
    this.positionElement = this.container.querySelector('#display-position');

    this._bindEvents();
  }

  /**
   * Bind event listeners
   * @private
   */
  _bindEvents() {
    this.selectElement.addEventListener('change', () => {
      const displayId = this.selectElement.value;
      this._handleDisplaySelect(displayId);
    });

    this.refreshButton.addEventListener('click', () => {
      this._refreshDisplayList();
    });
  }

  /**
   * Load the initial display list
   * @private
   */
  async _loadDisplayList() {
    try {
      this.displays = this.getDisplayList();
      this._populateDropdown();

      // Get current display info
      const currentDisplay = this.getDisplayInfo();
      if (currentDisplay) {
        this.currentDisplayId = currentDisplay.id;
        this._updateDisplayInfo(currentDisplay);
        this.selectElement.value = currentDisplay.id;
      }
    } catch (error) {
      console.error('Failed to load display list:', error);
      this._showError('Failed to load displays');
    }
  }

  /**
   * Refresh the display list
   * @private
   */
  async _refreshDisplayList() {
    if (this.isRefreshing) {
      return;
    }

    this.isRefreshing = true;
    this.selectElement.disabled = true;
    this.selectElement.innerHTML = '<option value="">Refreshing...</option>';
    this.refreshButton.classList.add('spinning');

    try {
      this.displays = this.getDisplayList();
      this._populateDropdown();

      // Restore selection
      if (this.currentDisplayId) {
        this.selectElement.value = this.currentDisplayId;
      }
    } catch (error) {
      console.error('Failed to refresh display list:', error);
      this._showError('Failed to refresh displays');
    } finally {
      this.isRefreshing = false;
      this.selectElement.disabled = false;
      this.refreshButton.classList.remove('spinning');
    }
  }

  /**
   * Populate the dropdown with available displays
   * @private
   */
  _populateDropdown() {
    if (this.displays.length === 0) {
      this.selectElement.innerHTML = '<option value="">No displays available</option>';
      this.selectElement.disabled = true;
      return;
    }

    this.selectElement.innerHTML = this.displays.map(display => `
      <option value="${display.id}" ${display.is_primary ? 'data-primary="true"' : ''}>
        ${this._formatDisplayOption(display)}
      </option>
    `).join('');

    this.selectElement.disabled = false;
  }

  /**
   * Format a display option for the dropdown
   * @private
   */
  _formatDisplayOption(display) {
    let label = `Display ${display.display_index}`;
    
    if (display.name) {
      label += ` (${display.name})`;
    }
    
    if (display.is_primary) {
      label += ' [Primary]';
    }
    
    label += ` - ${display.width}x${display.height}`;
    
    return label;
  }

  /**
   * Handle display selection change
   * @private
   */
  async _handleDisplaySelect(displayId) {
    if (!displayId || displayId === this.currentDisplayId) {
      return;
    }

    const display = this.displays.find(d => d.id === displayId);
    if (!display) {
      console.error('Selected display not found:', displayId);
      return;
    }

    this._updateDisplayInfo(display);
    this.currentDisplayId = displayId;

    // Notify parent component
    await this.onDisplaySelect(display);
  }

  /**
   * Update display information display
   * @private
   */
  _updateDisplayInfo(display) {
    this.resolutionElement.textContent = `${display.width}x${display.height}`;
    this.positionElement.textContent = `(${display.x}, ${display.y})`;
    this.displayInfo.classList.remove('hidden');
  }

  /**
   * Show error message
   * @private
   */
  _showError(message) {
    this.selectElement.innerHTML = `<option value="">${message}</option>`;
    this.selectElement.disabled = true;
  }

  /**
   * Public API: Programmatically select a display
   * @param {string} displayId - The display ID to select
   */
  selectDisplay(displayId) {
    if (!this.displays.find(d => d.id === displayId)) {
      console.warn('Display not found:', displayId);
      return;
    }
    
    this.selectElement.value = displayId;
    this._handleDisplaySelect(displayId);
  }

  /**
   * Public API: Get the currently selected display
   * @returns {Object|null} The current display object
   */
  getCurrentDisplay() {
    return this.displays.find(d => d.id === this.currentDisplayId) || null;
  }

  /**
   * Public API: Get the list of available displays
   * @returns {Array} Array of display objects
   */
  getDisplayList() {
    return [...this.displays];
  }

  /**
   * Public API: Enable/disable the selector
   * @param {boolean} enabled - Whether to enable the selector
   */
  setEnabled(enabled) {
    this.selectElement.disabled = !enabled;
    this.refreshButton.disabled = !enabled;
    
    if (!enabled) {
      this.selectElement.classList.add('disabled');
    } else {
      this.selectElement.classList.remove('disabled');
    }
  }

  /**
   * Public API: Update the display list (e.g., after hot-plug detection)
   * @param {Array} displays - New list of displays
   */
  updateDisplayList(displays) {
    this.displays = displays;
    this._populateDropdown();
    
    // Try to preserve current selection
    if (this.currentDisplayId && this.displays.find(d => d.id === this.currentDisplayId)) {
      this.selectElement.value = this.currentDisplayId;
    }
  }

  /**
   * Public API: Destroy the component and clean up
   */
  destroy() {
    this.selectElement.removeEventListener('change', this._handleDisplaySelect);
    this.refreshButton.removeEventListener('click', this._refreshDisplayList);
    this.container.innerHTML = '';
  }
}
