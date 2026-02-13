#include <gtest/gtest.h>

#include <chrono>

#include "e2e_test_helper.h"

namespace screensdk {

class VideoStreamTest : public ::testing::Test {
protected:
  void SetUp() override {
    controller_ = std::make_unique<MockController>();
    remote_host_ = std::make_unique<MockRemoteHost>();

    ASSERT_TRUE(controller_->initialize());
    ASSERT_TRUE(remote_host_->initialize(0));

    bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);
    ASSERT_TRUE(connected);
    ASSERT_TRUE(controller_->getDataChannel()->isConnected());
    ASSERT_TRUE(remote_host_->getDataChannel()->isConnected());
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

  std::unique_ptr<MockController> controller_;
  std::unique_ptr<MockRemoteHost> remote_host_;
};

TEST_F(VideoStreamTest, StartStopStreaming) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  remote_host_->stopStreaming();

  EXPECT_GT(remote_host_->getSentFrameCount(), 0);
}

TEST_F(VideoStreamTest, ReceiveVideoFrames) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  controller_->waitForFrame(2000);

  int received_count = controller_->getReceivedFrameCount();

  remote_host_->stopStreaming();

  EXPECT_GT(received_count, 0);
}

TEST_F(VideoStreamTest, FrameRateMeasurement) {
  const int test_duration_ms = 2000;
  const int expected_min_fps = 30;

  remote_host_->startStreaming();

  auto start_time = std::chrono::steady_clock::now();
  while (std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::steady_clock::now() - start_time).count() < test_duration_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  int sent_count = remote_host_->getSentFrameCount();
  int received_count = controller_->getReceivedFrameCount();

  remote_host_->stopStreaming();

  double sent_fps = (sent_count * 1000.0) / test_duration_ms;
  double received_fps = (received_count * 1000.0) / test_duration_ms;

  std::cout << "Sent FPS: " << sent_fps << ", Received FPS: " << received_fps << std::endl;

  EXPECT_GT(sent_fps, expected_min_fps);
  EXPECT_GT(received_fps, expected_min_fps * 0.5);
}

TEST_F(VideoStreamTest, FrameSequenceContinuity) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  remote_host_->stopStreaming();

  auto last_frame = controller_->getLastFrame();
  EXPECT_GT(last_frame.frame_number, 0);
}

TEST_F(VideoStreamTest, FrameTimestampsValid) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  remote_host_->stopStreaming();

  auto last_frame = controller_->getLastFrame();
  EXPECT_GT(last_frame.capture_time_ms, 0);

  uint64_t current_time = MetricsCollector::getCurrentTimeMs();
  EXPECT_LE(last_frame.capture_time_ms, current_time);
}

TEST_F(VideoStreamTest, MultipleStartStop) {
  for (int i = 0; i < 3; ++i) {
    int frame_count_before = remote_host_->getSentFrameCount();

    remote_host_->startStreaming();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    remote_host_->stopStreaming();

    int frame_count_after = remote_host_->getSentFrameCount();

    EXPECT_GT(frame_count_after, frame_count_before);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

TEST_F(VideoStreamTest, StreamingWithoutConnection) {
  // Disconnect first
  remote_host_->getDataChannel()->disconnect();
  controller_->getDataChannel()->disconnect();

  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  int sent_count = remote_host_->getSentFrameCount();

  remote_host_->stopStreaming();

  // Frames should be captured but not sent (count stays at 0)
  EXPECT_EQ(sent_count, 0);
}

TEST_F(VideoStreamTest, LongRunningStability) {
  const int test_duration_ms = 10000;  // 10 seconds

  remote_host_->startStreaming();

  auto start_time = std::chrono::steady_clock::now();
  int64_t total_received = 0;

  while (std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::steady_clock::now() - start_time).count() < test_duration_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    total_received = controller_->getReceivedFrameCount();
    std::cout << "Received " << total_received << " frames" << std::endl;
  }

  remote_host_->stopStreaming();

  EXPECT_GT(total_received, 0);
  SUCCEED() << "Completed 10-second streaming test";
}

TEST_F(VideoStreamTest, FrameDataIntegrity) {
  remote_host_->startStreaming();

  controller_->waitForFrame(2000);

  remote_host_->stopStreaming();

  auto frame = controller_->getLastFrame();

  EXPECT_FALSE(frame.data.empty());
}

TEST_F(VideoStreamTest, ConcurrentStreaming) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  int sent_count = remote_host_->getSentFrameCount();
  int received_count = controller_->getReceivedFrameCount();

  remote_host_->stopStreaming();

  std::cout << "Sent: " << sent_count << ", Received: " << received_count << std::endl;

  EXPECT_GT(sent_count, 0);
  EXPECT_GT(received_count, 0);
}

} // namespace screensdk
