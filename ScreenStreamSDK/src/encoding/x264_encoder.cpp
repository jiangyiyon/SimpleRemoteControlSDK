#include "screensdk/encoding/x264_encoder.h"
#include "screensdk/utils/error.h"

#undef max
#undef min

namespace screensdk {

X264EncoderImpl::X264EncoderImpl() = default;

X264EncoderImpl::~X264EncoderImpl() {
  std::lock_guard<std::mutex> lock(mutex_);

  // Close encoder first
  if (encoder_ != nullptr) {
    x264_encoder_close(encoder_);
    encoder_ = nullptr;
  }

  // Clean up pictures
  freePictures();

  // Clean up params
  if (param_ != nullptr) {
    x264_param_cleanup(param_);
    delete param_;
    param_ = nullptr;
  }
}

bool X264EncoderImpl::initialize(int width, int height, int fps,
                                 const std::string& config) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (initialized_) {
    return false;
  }

  if (width <= 0 || height <= 0 || fps <= 0) {
    return false;
  }

  width_ = width;
  height_ = height;
  fps_ = fps;

  param_ = new x264_param_t;
  x264_param_default(param_);

  if (!applyConfig(config)) {
    freePictures();
    x264_param_cleanup(param_);
    delete param_;
    param_ = nullptr;
    return false;
  }

  if (!setupParams()) {
    freePictures();
    x264_param_cleanup(param_);
    delete param_;
    param_ = nullptr;
    return false;
  }

  if (!allocatePictures()) {
    freePictures();
    x264_param_cleanup(param_);
    delete param_;
    param_ = nullptr;
    return false;
  }

  encoder_ = x264_encoder_open(param_);
  if (encoder_ == nullptr) {
    freePictures();
    x264_param_cleanup(param_);
    delete param_;
    param_ = nullptr;
    return false;
  }

  initialized_ = true;
  return true;
}

bool X264EncoderImpl::encode(const VideoFrameForTrans& frame,
                              uint8_t* output, size_t* output_size) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_ || encoder_ == nullptr) {
    return false;
  }

  if (frame.data == nullptr || frame.size == 0 ||
      output == nullptr || output_size == nullptr) {
    return false;
  }

  // Save original plane pointer to restore it after encoding
  uint8_t* original_plane = pic_in_->img.plane[0];

  pic_in_->i_pts = frame.timestamp_ms;
  pic_in_->img.plane[0] = frame.data;
  pic_in_->img.i_stride[0] = frame.stride > 0 ? frame.stride : (frame.width * 4);

  x264_nal_t* nals = nullptr;
  int i_nals = 0;
  int frame_size = x264_encoder_encode(encoder_, &nals, &i_nals, pic_in_, pic_out_);

  // Restore original plane pointer
  pic_in_->img.plane[0] = original_plane;

  if (frame_size < 0) {
    return false;
  }

  size_t total_size = 0;
  for (int i = 0; i < i_nals; ++i) {
    total_size += nals[i].i_payload;
  }

  if (total_size > *output_size) {
    return false;
  }

  size_t offset = 0;
  for (int i = 0; i < i_nals; ++i) {
    if (offset + nals[i].i_payload <= *output_size) {
      std::memcpy(output + offset, nals[i].p_payload, nals[i].i_payload);
      offset += nals[i].i_payload;
    }
  }

  *output_size = offset;
  return frame_size > 0;
}

void X264EncoderImpl::flush() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!initialized_ || encoder_ == nullptr) {
    return;
  }

  x264_nal_t* nals = nullptr;
  int i_nals = 0;
  x264_picture_t pic_out;

  while (x264_encoder_encode(encoder_, &nals, &i_nals, nullptr, &pic_out) > 0) {
    // Discard flushed frames
  }
}

EncoderType X264EncoderImpl::getType() const {
  return EncoderType::kSoftwareX264;
}

bool X264EncoderImpl::isAvailable() const {
  return true;
}

bool X264EncoderImpl::allocatePictures() {
  if (param_ == nullptr) {
    return false;
  }

  pic_in_ = new x264_picture_t;
  pic_out_ = new x264_picture_t;

  if (x264_picture_alloc(pic_in_, param_->i_csp, param_->i_width, param_->i_height) < 0) {
    delete pic_in_;
    delete pic_out_;
    pic_in_ = nullptr;
    pic_out_ = nullptr;
    return false;
  }

  x264_picture_init(pic_out_);

  return true;
}

void X264EncoderImpl::freePictures() {
  if (pic_in_ != nullptr) {
    x264_picture_clean(pic_in_);
    delete pic_in_;
    pic_in_ = nullptr;
  }
  if (pic_out_ != nullptr) {
    delete pic_out_;
    pic_out_ = nullptr;
  }
}

bool X264EncoderImpl::setupParams() {
  if (param_ == nullptr) {
    return false;
  }

  param_->i_csp = X264_CSP_BGRA;
  param_->i_width = width_;
  param_->i_height = height_;
  param_->i_fps_num = fps_;
  param_->i_fps_den = 1;

  param_->b_annexb = 1;
  param_->b_repeat_headers = 1;

  // GOP and RC params should be set by applyConfig
  // Only set defaults if not already set
  if (param_->i_keyint_max == 0) {
    param_->i_keyint_max = param_->i_fps_num;
    param_->i_keyint_min = param_->i_keyint_max / 2;
  }

  return true;
}

bool X264EncoderImpl::applyConfig(const std::string& config_json) {
  if (param_ == nullptr) {
    return false;
  }

  EncoderConfig config;
  if (!config_json.empty()) {
    config = EncoderConfig::fromJson(config_json);
  }

  if (!validateEncoderConfig(config)) {
    return false;
  }

  if (x264_param_default_preset(param_, config.preset.c_str(), config.tune.c_str()) < 0) {
    return false;
  }

  param_->i_width = config.width;
  param_->i_height = config.height;
  param_->i_fps_num = config.fps;
  param_->i_fps_den = 1;

  param_->i_keyint_max = config.gop_size;
  param_->i_keyint_min = std::max(1, config.gop_size / 2);

  param_->i_bframe = config.b_frames;

  param_->rc.i_bitrate = config.bitrate / 1000;
  param_->rc.i_vbv_max_bitrate = param_->rc.i_bitrate;
  param_->rc.i_vbv_buffer_size = param_->rc.i_bitrate / 4;

  if (x264_param_apply_profile(param_, "high") < 0) {
    return false;
  }

  return true;
}

} // namespace screensdk
