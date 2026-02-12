#include "screensdk/utils/metrics_collector.h"
#include <algorithm>

namespace screensdk {

MetricsCollector::MetricsCollector() {
  for (auto& samples : latency_samples_) {
    samples.reserve(kLatencySampleBufferSize);
  }
}

void MetricsCollector::recordLatency(LatencyType type, int latency_ms) {
  LatencySample sample;
  sample.timestamp_ms = getCurrentTimeMs();
  sample.latency_ms = latency_ms;
  sample.type = type;

  // Update average (exponential moving average)
  float alpha = 0.1f;  // Smoothing factor

  auto& samples = latency_samples_[static_cast<size_t>(type)];
  samples.push_back(sample);
  if (samples.size() > kLatencySampleBufferSize) {
    samples.erase(samples.begin());
  }

  switch (type) {
    case LatencyType::kCaptureToEncode: {
      int current = capture_to_encode_avg_.load();
      capture_to_encode_avg_.store(
          static_cast<int>(current + alpha * (latency_ms - current)));
      break;
    }
    case LatencyType::kEncodeToTransport: {
      int current = encode_to_transport_avg_.load();
      encode_to_transport_avg_.store(
          static_cast<int>(current + alpha * (latency_ms - current)));
      break;
    }
    case LatencyType::kTransportToDecode: {
      int current = transport_to_decode_avg_.load();
      transport_to_decode_avg_.store(
          static_cast<int>(current + alpha * (latency_ms - current)));
      break;
    }
    case LatencyType::kDecodeToDisplay: {
      int current = decode_to_display_avg_.load();
      decode_to_display_avg_.store(
          static_cast<int>(current + alpha * (latency_ms - current)));
      break;
    }
    case LatencyType::kTotalLatency: {
      int current = total_latency_avg_.load();
      total_latency_avg_.store(
          static_cast<int>(current + alpha * (latency_ms - current)));
      break;
    }
  }
}

int MetricsCollector::getAverageLatency(LatencyType type) const {
  switch (type) {
    case LatencyType::kCaptureToEncode:
      return capture_to_encode_avg_.load();
    case LatencyType::kEncodeToTransport:
      return encode_to_transport_avg_.load();
    case LatencyType::kTransportToDecode:
      return transport_to_decode_avg_.load();
    case LatencyType::kDecodeToDisplay:
      return decode_to_display_avg_.load();
    case LatencyType::kTotalLatency:
      return total_latency_avg_.load();
  }
  return 0;
}

int MetricsCollector::getMaxLatency(LatencyType type) const {
  const auto& samples = latency_samples_[static_cast<size_t>(type)];
  if (samples.empty()) return 0;

  int max_latency = 0;
  for (const auto& sample : samples) {
    if (sample.latency_ms > max_latency) {
      max_latency = sample.latency_ms;
    }
  }
  return max_latency;
}

void MetricsCollector::recordFrame() {
  frame_count_++;

  uint64_t now = getCurrentTimeMs();
  if (last_fps_update_ == 0) {
    last_fps_update_ = now;
    return;
  }

  uint64_t elapsed = now - last_fps_update_;
  if (elapsed >= 1000) {  // Update every second
    double current_fps = (frame_count_ * 1000.0) / elapsed;
    fps_.store(current_fps);
    frame_count_ = 0;
    last_fps_update_ = now;
  }
}

double MetricsCollector::getFps() const {
  // If no FPS update yet, calculate based on current frame count and elapsed time
  if (fps_.load() == 0.0 && last_fps_update_ != 0) {
    uint64_t now = getCurrentTimeMs();
    uint64_t elapsed = now - last_fps_update_;
    if (elapsed > 0) {
      return (frame_count_ * 1000.0) / elapsed;
    }
  }
  return fps_.load();
}

std::vector<LatencySample> MetricsCollector::getRecentLatencySamples(
    LatencyType type, size_t max_samples) const {
  const auto& samples = latency_samples_[static_cast<size_t>(type)];
  if (samples.empty()) return {};

  size_t count = std::min(max_samples, samples.size());
  return std::vector<LatencySample>(
      samples.end() - count, samples.end());
}

int MetricsCollector::getErrorCount(MetricCategory category) const {
  switch (category) {
    case MetricCategory::kCapture:
      return capture_errors_.load();
    case MetricCategory::kEncoding:
      return encoding_errors_.load();
    case MetricCategory::kTransport:
      return transport_errors_.load();
    case MetricCategory::kDecoding:
      return decoding_errors_.load();
    case MetricCategory::kInput:
      return input_errors_.load();
  }
  return 0;
}

void MetricsCollector::recordError(MetricCategory category) {
  switch (category) {
    case MetricCategory::kCapture:
      capture_errors_.fetch_add(1);
      break;
    case MetricCategory::kEncoding:
      encoding_errors_.fetch_add(1);
      break;
    case MetricCategory::kTransport:
      transport_errors_.fetch_add(1);
      break;
    case MetricCategory::kDecoding:
      decoding_errors_.fetch_add(1);
      break;
    case MetricCategory::kInput:
      input_errors_.fetch_add(1);
      break;
  }
}

void MetricsCollector::reset() {
  capture_to_encode_avg_.store(0);
  encode_to_transport_avg_.store(0);
  transport_to_decode_avg_.store(0);
  decode_to_display_avg_.store(0);
  total_latency_avg_.store(0);
  fps_.store(0.0);
  bytes_sent_.store(0);
  bytes_received_.store(0);
  capture_errors_.store(0);
  encoding_errors_.store(0);
  transport_errors_.store(0);
  decoding_errors_.store(0);
  input_errors_.store(0);
  frame_count_ = 0;
  last_fps_update_ = 0;

  for (auto& samples : latency_samples_) {
    samples.clear();
  }
}

uint64_t MetricsCollector::getCurrentTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace screensdk
