#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

namespace screensdk {

/**
 * @brief Latency measurement points
 */
enum class LatencyType {
  kCaptureToEncode,
  kEncodeToTransport,
  kTransportToDecode,
  kDecodeToDisplay,
  kTotalLatency
};

/**
 * @brief Metrics category for different system components
 */
enum class MetricCategory {
  kCapture,
  kEncoding,
  kTransport,
  kDecoding,
  kInput
};

/**
 * @brief Sample for latency measurements
 */
struct LatencySample {
  uint64_t timestamp_ms;
  int latency_ms;
  LatencyType type;
};

/**
 * @brief Metrics collector for real-time performance monitoring
 * 
 * FR-014: System MUST display real-time latency metrics
 * FR-015: System MUST log errors with exponential backoff retry
 */
class MetricsCollector {
public:
  MetricsCollector();
  ~MetricsCollector() = default;

  // Disable copy and move
  MetricsCollector(const MetricsCollector&) = delete;
  MetricsCollector& operator=(const MetricsCollector&) = delete;
  MetricsCollector(MetricsCollector&&) = delete;
  MetricsCollector& operator=(MetricsCollector&&) = delete;

  /**
   * @brief Record a latency measurement
   */
  void recordLatency(LatencyType type, int latency_ms);

  /**
   * @brief Get average latency for a specific type
   */
  int getAverageLatency(LatencyType type) const;

  /**
   * @brief Get maximum latency for a specific type
   */
  int getMaxLatency(LatencyType type) const;

  /**
   * @brief Get current frames per second
   */
  double getFps() const;

  /**
   * @brief Update FPS measurement
   */
  void recordFrame();

  /**
   * @brief Get recent latency samples (last N samples)
   */
  std::vector<LatencySample> getRecentLatencySamples(
      LatencyType type, size_t max_samples) const;

  /**
   * @brief Get total bytes sent via transport
   */
  uint64_t getBytesSent() const { return bytes_sent_.load(); }

  /**
   * @brief Get total bytes received via transport
   */
  uint64_t getBytesReceived() const { return bytes_received_.load(); }

  /**
   * @brief Record bytes sent
   */
  void recordBytesSent(uint64_t bytes) {
    bytes_sent_.fetch_add(bytes);
  }

  /**
   * @brief Record bytes received
   */
  void recordBytesReceived(uint64_t bytes) {
    bytes_received_.fetch_add(bytes);
  }

  /**
   * @brief Get error count for a category
   */
  int getErrorCount(MetricCategory category) const;

  /**
   * @brief Record an error
   */
  void recordError(MetricCategory category);

  /**
   * @brief Reset all metrics
   */
  void reset();

  /**
   * @brief Get current timestamp in milliseconds
   */
  static uint64_t getCurrentTimeMs();

private:
  // Latency tracking
  static constexpr size_t kLatencySampleBufferSize = 1000;
  std::atomic<int> capture_to_encode_avg_{0};
  std::atomic<int> encode_to_transport_avg_{0};
  std::atomic<int> transport_to_decode_avg_{0};
  std::atomic<int> decode_to_display_avg_{0};
  std::atomic<int> total_latency_avg_{0};

  // FPS tracking
  std::atomic<double> fps_{0.0};
  uint64_t last_fps_update_{0};
  uint32_t frame_count_{0};

  // Network tracking
  std::atomic<uint64_t> bytes_sent_{0};
  std::atomic<uint64_t> bytes_received_{0};

  // Error tracking
  std::atomic<int> capture_errors_{0};
  std::atomic<int> encoding_errors_{0};
  std::atomic<int> transport_errors_{0};
  std::atomic<int> decoding_errors_{0};
  std::atomic<int> input_errors_{0};

  // Latency samples history (simplified, would need proper lock in production)
  mutable std::vector<LatencySample> latency_samples_[5];
};

} // namespace screensdk
