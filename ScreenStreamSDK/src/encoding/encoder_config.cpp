#include "screensdk/encoding/encoder_config.h"
#include "../../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

namespace screensdk {

// Define JSON serialization using non-intrusive macro
// This keeps the header file clean from nlohmann/json dependency
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
  EncoderConfig,
  width, height, fps,
  gop_size, b_frames, bitrate, quality,
  preset, tune,
  use_hardware_encoder, allow_software_fallback
);

std::string EncoderConfig::toJson() const {
  return json(*this).dump();
}

EncoderConfig EncoderConfig::fromJson(const std::string& json_str) {
  try {
    return json::parse(json_str).get<EncoderConfig>();
  } catch (const json::exception&) {
    return getDefault();
  }
}

EncoderConfig EncoderConfig::getDefault() {
  return getLowLatency();
}

EncoderConfig EncoderConfig::getLowLatency() {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;
  config.gop_size = 1;
  config.b_frames = 0;
  config.bitrate = 5000000;
  config.quality = 23;
  config.preset = "ultrafast";
  config.tune = "zerolatency";
  config.use_hardware_encoder = true;
  config.allow_software_fallback = true;
  return config;
}

EncoderConfig EncoderConfig::getHighQuality() {
  EncoderConfig config;
  config.width = 1920;
  config.height = 1080;
  config.fps = 60;
  config.gop_size = 30;
  config.b_frames = 3;
  config.bitrate = 8000000;
  config.quality = 18;
  config.preset = "fast";
  config.tune = "zerolatency";
  config.use_hardware_encoder = true;
  config.allow_software_fallback = true;
  return config;
}

bool validateEncoderConfig(const EncoderConfig& config) {
  // Validate resolution
  if (config.width <= 0 || config.width > 4096) return false;
  if (config.height <= 0 || config.height > 2160) return false;

  // Validate FPS
  if (config.fps < 1 || config.fps > 120) return false;

  // Validate GOP
  if (config.gop_size < 1 || config.gop_size > 600) return false;

  // Validate B-frames
  if (config.b_frames < 0 || config.b_frames > 16) return false;

  // Validate bitrate
  if (config.bitrate < 1000000 || config.bitrate > 20000000) return false;

  // Validate quality
  if (config.quality < 0 || config.quality > 51) return false;

  return true;
}

} // namespace screensdk
