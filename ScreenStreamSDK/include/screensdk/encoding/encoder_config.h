#pragma once

#include <cstdint>
#include <string>

namespace screensdk {

/**
 * @brief Encoder configuration for low-latency streaming
 * 
 * T022: Implement low-latency encoder configuration (GOP=1, B-frames=0)
 * FR-001: 60±5 frames per second
 * FR-003: Hardware encoding with software fallback
 * 
 * Configuration optimized for low-latency real-time streaming:
 * - GOP=1 for minimal latency
 * - B-frames=0 for instant encoding
 * - High bitrate for quality
 */
struct EncoderConfig {
  // Resolution
  int width{1920};
  int height{1080};

  // Frame rate
  int fps{60};

  // GOP configuration
  int gop_size{1};         // I-frame interval (1 = every frame)
  int b_frames{0};         // B-frames for latency optimization

  // Bitrate
  int bitrate{5000000};     // 5 Mbps default

  // Quality settings
  int quality{23};          // CRF value (lower = higher quality)

  // Preset (for x264)
  std::string preset{"ultrafast"};

  // Tune mode
  std::string tune{"zerolatency"};

  // Hardware encoder settings
  bool use_hardware_encoder{true};
  bool allow_software_fallback{true};

  /**
   * @brief Get configuration as JSON string
   */
  std::string toJson() const;

  /**
   * @brief Parse configuration from JSON string
   */
  static EncoderConfig fromJson(const std::string& json);

  /**
   * @brief Get default configuration
   */
  static EncoderConfig getDefault();

  /**
   * @brief Get low-latency configuration
   */
  static EncoderConfig getLowLatency();

  /**
   * @brief Get high-quality configuration
   */
  static EncoderConfig getHighQuality();
};

/**
 * @brief Validate encoder configuration
 */
bool validateEncoderConfig(const EncoderConfig& config);

} // namespace screensdk
