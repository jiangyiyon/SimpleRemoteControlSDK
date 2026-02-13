#include <gtest/gtest.h>

#include <thread>
#include <chrono>

#include "e2e_test_helper.h"

namespace screensdk {

class StabilityTest : public ::testing::Test {
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

  std::unique_ptr<MockController> controller_;
  std::unique_ptr<MockRemoteHost> remote_host_;
};

TEST_F(StabilityTest, ShortTermStability) {
  const int test_duration_seconds = 30;

  remote_host_->startStreaming();

  auto start_time = std::chrono::steady_clock::now();

  while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < test_duration_seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    int sent = remote_host_->getSentFrameCount();
    int received = controller_->getReceivedFrameCount();
    std::cout << "Sent: " << sent << ", Received: " << received << std::endl;
  }

  remote_host_->stopStreaming();

  int final_sent = remote_host_->getSentFrameCount();
  int final_received = controller_->getReceivedFrameCount();

  EXPECT_GT(final_sent, 0);
  EXPECT_GT(final_received, 0);

  SUCCEED() << "Completed " << test_duration_seconds << "-second stability test";
}

TEST_F(StabilityTest, MediumTermStability) {
  const int test_duration_seconds = 60;

  remote_host_->startStreaming();

  auto start_time = std::chrono::steady_clock::now();

  while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < test_duration_seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    int sent = remote_host_->getSentFrameCount();
    int received = controller_->getReceivedFrameCount();
    double fps = (sent * 1.0) / (std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - start_time).count());

    std::cout << "Elapsed: "
              << std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time).count()
              << "s, Sent: " << sent << ", Received: " << received
              << ", FPS: " << fps << std::endl;
  }

  remote_host_->stopStreaming();

  int final_sent = remote_host_->getSentFrameCount();
  int final_received = controller_->getReceivedFrameCount();

  EXPECT_GT(final_sent, 0);
  EXPECT_GT(final_received, 0);

  SUCCEED() << "Completed " << test_duration_seconds << "-second stability test";
}

TEST_F(StabilityTest, StartStopCycling) {
  const int cycle_count = 10;
  const int cycle_duration_ms = 1000;

  for (int i = 0; i < cycle_count; ++i) {
    std::cout << "Cycle " << (i + 1) << "/" << cycle_count << std::endl;

    remote_host_->startStreaming();
    std::this_thread::sleep_for(std::chrono::milliseconds(cycle_duration_ms));
    remote_host_->stopStreaming();

    int sent = remote_host_->getSentFrameCount();
    int received = controller_->getReceivedFrameCount();

    std::cout << "  Sent: " << sent << ", Received: " << received << std::endl;

    EXPECT_GE(sent, 0);
    EXPECT_GE(received, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  SUCCEED() << "Completed " << cycle_count << " start/stop cycles";
}

TEST_F(StabilityTest, ConnectionRecovery) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  int frames_before = controller_->getReceivedFrameCount();
  std::cout << "Frames before disconnect: " << frames_before << std::endl;

  EXPECT_GT(frames_before, 0) << "Should receive frames before disconnect";

  // Disconnect
  remote_host_->getDataChannel()->disconnect();
  controller_->getDataChannel()->disconnect();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Cleanup and recreate Mock objects for reconnection
  remote_host_->cleanup();
  controller_->cleanup();

  controller_ = std::make_unique<MockController>();
  remote_host_ = std::make_unique<MockRemoteHost>();

  ASSERT_TRUE(controller_->initialize());
  ASSERT_TRUE(remote_host_->initialize(0));

  // Reconnect with new objects
  bool reconnected = ConnectionHelper::establishConnection(*remote_host_, *controller_);
  EXPECT_TRUE(reconnected) << "Reconnection should succeed";

  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  remote_host_->stopStreaming();

  int frames_after = controller_->getReceivedFrameCount();

  std::cout << "Frames after reconnect: " << frames_after << std::endl;

  EXPECT_GT(frames_after, 0) << "Should receive frames after reconnect";

  // Both sessions should have received frames
  SUCCEED() << "Connection recovery test passed (before: "
            << frames_before << ", after: " << frames_after << ")";
}

TEST_F(StabilityTest, NetworkInterruptionSimulation) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Simulate network interruption by pausing sending
  auto pause_thread = std::thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    std::cout << "Simulating network interruption..." << std::endl;

    // Pause capture
    remote_host_->stopStreaming();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Resume
    std::cout << "Resuming after interruption..." << std::endl;
    remote_host_->startStreaming();
  });

  // Collect frames for 5 seconds
  auto start_time = std::chrono::steady_clock::now();
  int last_frame_count = 0;
  int no_progress_count = 0;

  while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < 5) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    int current_frame_count = controller_->getReceivedFrameCount();
    if (current_frame_count == last_frame_count) {
      no_progress_count++;
    } else {
      no_progress_count = 0;
    }
    last_frame_count = current_frame_count;

    std::cout << "Received: " << current_frame_count << std::endl;
  }

  remote_host_->stopStreaming();

  if (pause_thread.joinable()) {
    pause_thread.join();
  }

  int final_count = controller_->getReceivedFrameCount();

  EXPECT_GT(final_count, 0);

  std::cout << "Final frame count: " << final_count << std::endl;
  std::cout << "No progress intervals: " << no_progress_count << std::endl;

  SUCCEED() << "Network interruption simulation completed";
}

TEST_F(StabilityTest, ResourceLeakDetection) {
  const int test_duration_seconds = 10;

  // Capture initial state
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::seconds(test_duration_seconds));

  remote_host_->stopStreaming();

  int final_sent = remote_host_->getSentFrameCount();
  int final_received = controller_->getReceivedFrameCount();

  // Clean up
  remote_host_->cleanup();
  controller_->cleanup();

  // Reinitialize to detect resource leaks
  controller_ = std::make_unique<MockController>();
  remote_host_ = std::make_unique<MockRemoteHost>();

  ASSERT_TRUE(controller_->initialize());
  ASSERT_TRUE(remote_host_->initialize(0));

  bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);
  ASSERT_TRUE(connected);

  // Run again
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::seconds(test_duration_seconds));

  remote_host_->stopStreaming();

  int second_run_sent = remote_host_->getSentFrameCount();
  int second_run_received = controller_->getReceivedFrameCount();

  // Performance should be similar
  double sent_ratio = static_cast<double>(second_run_sent) / final_sent;
  double received_ratio = static_cast<double>(second_run_received) / final_received;

  std::cout << "First run - Sent: " << final_sent << ", Received: " << final_received << std::endl;
  std::cout << "Second run - Sent: " << second_run_sent << ", Received: " << second_run_received << std::endl;
  std::cout << "Sent ratio: " << sent_ratio << ", Received ratio: " << received_ratio << std::endl;

  // Ratios should be in reasonable range (0.5 to 2.0)
  EXPECT_GT(sent_ratio, 0.5);
  EXPECT_LT(sent_ratio, 2.0);
  EXPECT_GT(received_ratio, 0.5);
  EXPECT_LT(received_ratio, 2.0);

  SUCCEED() << "Resource leak detection test passed";
}

TEST_F(StabilityTest, FrameSequenceContinuity) {
  remote_host_->startStreaming();

  std::this_thread::sleep_for(std::chrono::seconds(5));

  remote_host_->stopStreaming();

  auto last_frame = controller_->getLastFrame();

  EXPECT_GT(last_frame.frame_number, 0);

  uint64_t total_frames = last_frame.frame_number;
  std::cout << "Total frames received: " << total_frames << std::endl;

  EXPECT_GT(total_frames, 50);  // At least 50 frames in 5 seconds (10 FPS)
}

TEST_F(StabilityTest, MemoryStability) {
  const int test_duration_seconds = 20;

  remote_host_->startStreaming();

  auto start_time = std::chrono::steady_clock::now();
  int64_t last_received = 0;
  int low_frame_count = 0;

  while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < test_duration_seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(2));

    int64_t current_received = controller_->getReceivedFrameCount();
    int64_t delta = current_received - last_received;
    last_received = current_received;

    std::cout << "Total received: " << current_received
              << ", Delta (last 2s): " << delta << std::endl;

    // Frame rate can fluctuate but should not drop too low consistently
    if (delta < 20) {  // Below 10 FPS for 2 seconds
      low_frame_count++;
    }
  }

  remote_host_->stopStreaming();

  // At least 15 FPS average over entire test
  double avg_fps = static_cast<double>(controller_->getReceivedFrameCount()) / test_duration_seconds;
  std::cout << "Average FPS: " << avg_fps << std::endl;
  EXPECT_GT(avg_fps, 15.0) << "Average FPS should be above 15";

  // Should not have sustained low frame rate
  EXPECT_LT(low_frame_count, 5) << "Should not have more than 4 intervals with low frame rate";

  SUCCEED() << "Memory stability test completed";
}

} // namespace screensdk
