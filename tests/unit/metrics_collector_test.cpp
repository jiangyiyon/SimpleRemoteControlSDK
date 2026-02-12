#include <gtest/gtest.h>
#include "screensdk/utils/metrics_collector.h"

using namespace screensdk;

class MetricsCollectorTest : public ::testing::Test {
protected:
  void SetUp() override {
    metrics_.reset();
  }

  MetricsCollector metrics_;
};

TEST_F(MetricsCollectorTest, RecordLatency) {
  metrics_.recordLatency(LatencyType::kTotalLatency, 30);
  metrics_.recordLatency(LatencyType::kTotalLatency, 40);
  metrics_.recordLatency(LatencyType::kTotalLatency, 35);

  EXPECT_GT(metrics_.getAverageLatency(LatencyType::kTotalLatency), 0);
}

TEST_F(MetricsCollectorTest, RecordFrameUpdatesFps) {
  // Simulate frames for over 1 second to trigger FPS calculation
  for (int i = 0; i < 70; ++i) {
    metrics_.recordFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60fps
  }

  // FPS calculation depends on actual elapsed time due to sleep inaccuracies
  // On Windows, sleep_for is not precise and may sleep longer than requested
  EXPECT_GT(metrics_.getFps(), 20.0);  // Lower bound for test reliability
  EXPECT_LT(metrics_.getFps(), 70.0);  // Upper bound to catch errors
}

TEST_F(MetricsCollectorTest, RecordBytesSentReceived) {
  metrics_.recordBytesSent(1024);
  metrics_.recordBytesReceived(2048);

  EXPECT_EQ(metrics_.getBytesSent(), 1024);
  EXPECT_EQ(metrics_.getBytesReceived(), 2048);
}

TEST_F(MetricsCollectorTest, RecordError) {
  metrics_.recordError(MetricCategory::kEncoding);
  metrics_.recordError(MetricCategory::kEncoding);
  metrics_.recordError(MetricCategory::kCapture);

  EXPECT_EQ(metrics_.getErrorCount(MetricCategory::kEncoding), 2);
  EXPECT_EQ(metrics_.getErrorCount(MetricCategory::kCapture), 1);
  EXPECT_EQ(metrics_.getErrorCount(MetricCategory::kTransport), 0);
}

TEST_F(MetricsCollectorTest, GetMaxLatency) {
  metrics_.recordLatency(LatencyType::kCaptureToEncode, 10);
  metrics_.recordLatency(LatencyType::kCaptureToEncode, 50);
  metrics_.recordLatency(LatencyType::kCaptureToEncode, 30);

  EXPECT_EQ(metrics_.getMaxLatency(LatencyType::kCaptureToEncode), 50);
}

TEST_F(MetricsCollectorTest, GetRecentLatencySamples) {
  for (int i = 0; i < 10; ++i) {
    metrics_.recordLatency(LatencyType::kEncodeToTransport, i * 10);
  }

  auto samples = metrics_.getRecentLatencySamples(
      LatencyType::kEncodeToTransport, 3);

  EXPECT_EQ(samples.size(), 3);
  EXPECT_EQ(samples[0].latency_ms, 70);
  EXPECT_EQ(samples[1].latency_ms, 80);
  EXPECT_EQ(samples[2].latency_ms, 90);
}

TEST_F(MetricsCollectorTest, Reset) {
  metrics_.recordLatency(LatencyType::kTotalLatency, 30);
  metrics_.recordFrame();
  metrics_.recordError(MetricCategory::kCapture);
  metrics_.recordBytesSent(1000);

  metrics_.reset();

  EXPECT_EQ(metrics_.getAverageLatency(LatencyType::kTotalLatency), 0);
  EXPECT_EQ(metrics_.getFps(), 0.0);
  EXPECT_EQ(metrics_.getErrorCount(MetricCategory::kCapture), 0);
  EXPECT_EQ(metrics_.getBytesSent(), 0);
}
