#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <vector>

#include "e2e_test_helper.h"

namespace screensdk {

class LatencyTest : public ::testing::Test {
protected:
  void SetUp() override {
    controller_ = std::make_unique<MockController>();
    remote_host_ = std::make_unique<MockRemoteHost>();

    ASSERT_TRUE(controller_->initialize());
    ASSERT_TRUE(remote_host_->initialize(0));

    bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);
    ASSERT_TRUE(connected);
  }

  void TearDown() override {
    if (remote_host_) {
      remote_host_->stopStreaming();
      remote_host_->cleanup();
      remote_host_.reset();
    }

    if (controller_) {
      controller_->cleanup();
      controller_.reset();
    }
  }

  std::vector<uint64_t> collectLatencySamples(int sample_count, int sample_interval_ms) {
    std::vector<uint64_t> latencies;
    latencies.reserve(sample_count);

    remote_host_->startStreaming();

    auto start_time = std::chrono::steady_clock::now();

    while (latencies.size() < static_cast<size_t>(sample_count)) {
      controller_->waitForFrame(sample_interval_ms * 2);

      uint64_t latency = controller_->getLastFrameLatency();
      if (latency > 0) {
        latencies.push_back(latency);
      }

      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();

      if (elapsed > sample_count * sample_interval_ms * 2) {
        break;
      }
    }

    remote_host_->stopStreaming();

    return latencies;
  }

  void printLatencyStatistics(const std::vector<uint64_t>& latencies) {
    if (latencies.empty()) {
      std::cout << "No latency samples collected" << std::endl;
      return;
    }

    uint64_t min_latency = *std::min_element(latencies.begin(), latencies.end());
    uint64_t max_latency = *std::max_element(latencies.begin(), latencies.end());

    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg_latency = sum / latencies.size();

    std::vector<uint64_t> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());
    double median = sorted[sorted.size() / 2];

    int p95_index = static_cast<int>(sorted.size() * 0.95);
    uint64_t p95_latency = sorted[p95_index];

    int p99_index = static_cast<int>(sorted.size() * 0.99);
    uint64_t p99_latency = sorted[p99_index];

    std::cout << "Latency Statistics (ms):" << std::endl;
    std::cout << "  Samples: " << latencies.size() << std::endl;
    std::cout << "  Min: " << min_latency << std::endl;
    std::cout << "  Max: " << max_latency << std::endl;
    std::cout << "  Avg: " << avg_latency << std::endl;
    std::cout << "  Median: " << median << std::endl;
    std::cout << "  P95: " << p95_latency << std::endl;
    std::cout << "  P99: " << p99_latency << std::endl;
  }

  std::unique_ptr<MockController> controller_;
  std::unique_ptr<MockRemoteHost> remote_host_;
};

TEST_F(LatencyTest, MeasureSingleFrameLatency) {
  remote_host_->startStreaming();

  controller_->waitForFrame(2000);

  remote_host_->stopStreaming();

  uint64_t latency = controller_->getLastFrameLatency();

  EXPECT_GT(latency, 0);
  std::cout << "Single frame latency: " << latency << " ms" << std::endl;
}

TEST_F(LatencyTest, MeasureAverageLatency) {
  const int sample_count = 10;
  const int sample_interval_ms = 100;

  auto latencies = collectLatencySamples(sample_count, sample_interval_ms);

  ASSERT_GT(latencies.size(), 0);

  printLatencyStatistics(latencies);

  double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
  double avg_latency = sum / latencies.size();

  // Note: 30ms target is for LAN with optimized path.
  // Local loopback should be faster, but we use a relaxed threshold.
  EXPECT_LT(avg_latency, 100);

  SUCCEED();
}

TEST_F(LatencyTest, LatencyConsistency) {
  const int sample_count = 30;
  const int sample_interval_ms = 100;

  auto latencies = collectLatencySamples(sample_count, sample_interval_ms);

  ASSERT_GT(latencies.size(), 10);

  printLatencyStatistics(latencies);

  uint64_t min_latency = *std::min_element(latencies.begin(), latencies.end());
  uint64_t max_latency = *std::max_element(latencies.begin(), latencies.end());

  uint64_t variance = max_latency - min_latency;

  std::cout << "Latency variance: " << variance << " ms" << std::endl;

  // Expect some consistency (variance should not be too large)
  EXPECT_LT(variance, 500);
}

TEST_F(LatencyTest, LatencyUnderLoad) {
  const int sample_count = 50;
  const int sample_interval_ms = 50;  // Higher frequency

  auto latencies = collectLatencySamples(sample_count, sample_interval_ms);

  ASSERT_GT(latencies.size(), 20);

  printLatencyStatistics(latencies);

  // Check P95 latency (95th percentile)
  std::vector<uint64_t> sorted = latencies;
  std::sort(sorted.begin(), sorted.end());

  int p95_index = static_cast<int>(sorted.size() * 0.95);
  uint64_t p95_latency = sorted[p95_index];

  std::cout << "P95 latency under load: " << p95_latency << " ms" << std::endl;

  EXPECT_LT(p95_latency, 200);
}

TEST_F(LatencyTest, LongTermLatencyStability) {
  const int sample_count = 100;
  const int sample_interval_ms = 100;

  auto latencies = collectLatencySamples(sample_count, sample_interval_ms);

  ASSERT_GT(latencies.size(), 50);

  printLatencyStatistics(latencies);

  // Check if latency remains stable over time
  std::vector<uint64_t> first_half(latencies.begin(), latencies.begin() + latencies.size() / 2);
  std::vector<uint64_t> second_half(latencies.begin() + latencies.size() / 2, latencies.end());

  double sum_first = std::accumulate(first_half.begin(), first_half.end(), 0.0);
  double avg_first = sum_first / first_half.size();

  double sum_second = std::accumulate(second_half.begin(), second_half.end(), 0.0);
  double avg_second = sum_second / second_half.size();

  double drift = std::abs(avg_second - avg_first);

  std::cout << "First half avg: " << avg_first << " ms" << std::endl;
  std::cout << "Second half avg: " << avg_second << " ms" << std::endl;
  std::cout << "Latency drift: " << drift << " ms" << std::endl;

  // Latency should not drift significantly over time
  EXPECT_LT(drift, 50);
}

TEST_F(LatencyTest, LatencyPercentiles) {
  const int sample_count = 100;
  const int sample_interval_ms = 50;

  auto latencies = collectLatencySamples(sample_count, sample_interval_ms);

  ASSERT_GT(latencies.size(), 50);

  printLatencyStatistics(latencies);

  std::vector<uint64_t> sorted = latencies;
  std::sort(sorted.begin(), sorted.end());

  uint64_t p50 = sorted[sorted.size() * 0.50];
  uint64_t p90 = sorted[sorted.size() * 0.90];
  uint64_t p95 = sorted[sorted.size() * 0.95];
  uint64_t p99 = sorted[sorted.size() * 0.99];

  std::cout << "P50: " << p50 << " ms" << std::endl;
  std::cout << "P90: " << p90 << " ms" << std::endl;
  std::cout << "P95: " << p95 << " ms" << std::endl;
  std::cout << "P99: " << p99 << " ms" << std::endl;

  // All percentiles should be reasonable
  EXPECT_LT(p99, 500);
}

TEST_F(LatencyTest, LatencyWithMultipleStreams) {
  // Test if adding data traffic affects video latency
  const int sample_count = 30;
  const int sample_interval_ms = 100;

  remote_host_->startStreaming();

  // Send additional data traffic
  std::thread traffic_thread([&]() {
    for (int i = 0; i < 50; ++i) {
      if (remote_host_->getDataChannel()->isConnected()) {
        std::vector<uint8_t> dummy_data(1024);
        remote_host_->getDataChannel()->send(dummy_data);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  std::vector<uint64_t> latencies;
  auto start_time = std::chrono::steady_clock::now();

  while (latencies.size() < static_cast<size_t>(sample_count)) {
    controller_->waitForFrame(sample_interval_ms * 2);

    uint64_t latency = controller_->getLastFrameLatency();
    if (latency > 0) {
      latencies.push_back(latency);
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time).count();

    if (elapsed > sample_count * sample_interval_ms * 3) {
      break;
    }
  }

  remote_host_->stopStreaming();

  if (traffic_thread.joinable()) {
    traffic_thread.join();
  }

  ASSERT_GT(latencies.size(), 10);

  printLatencyStatistics(latencies);

  double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
  double avg_latency = sum / latencies.size();

  std::cout << "Average latency with traffic: " << avg_latency << " ms" << std::endl;

  EXPECT_LT(avg_latency, 150);
}

} // namespace screensdk
