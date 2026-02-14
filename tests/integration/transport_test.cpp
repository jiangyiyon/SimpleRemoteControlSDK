#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <future>

#include "screensdk/transport/data_channel.h"
#include "screensdk/transport/sdp_renegotiation.h"

namespace screensdk {

/**
 * @brief Integration test for WebRTC transport layer
 *
 * Tests the complete transport pipeline:
 * DataChannel (WebRTC) ↔ SDP renegotiation ↔ Message transmission
 *
 * Key Integration Points:
 * - DataChannel P2P connection establishment (SDP + ICE exchange)
 * - Bidirectional message transmission
 * - SDP renegotiation for display switching
 * - Connection state management
 * - Error handling and recovery
 *
 * Scenarios:
 * - Complete connection establishment (offer/answer + ICE exchange)
 * - Bidirectional data transmission
 * - Connection state transitions
 * - SDP renegotiation flow
 * - Connection disconnect and reconnect
 * - Large message handling
 * - Rapid message transmission
 */
class TransportTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create DataChannel config for testing
    config_.label = "test_channel";
    config_.protocol = "test_protocol";
    config_.ordered = true;
    config_.maxRetransmits = 3;

    // Create controller and controlled DataChannels
    controller_ = new DataChannel(config_);
    controlled_ = new DataChannel(config_);

    // Setup callbacks for signaling simulation
    setupSignaling();
  }

  void TearDown() override {
    if (controller_ != nullptr) {
      controller_->disconnect();
      delete controller_;
      controller_ = nullptr;
    }

    if (controlled_ != nullptr) {
      controlled_->disconnect();
      delete controlled_;
      controlled_ = nullptr;
    }
  }

  /**
   * @brief Setup signaling callbacks for SDP and ICE exchange
   */
  void setupSignaling() {
    // Controller callbacks
    controller_->onLocalDescription([this](const std::string& sdp) {
      controller_local_sdp_ = sdp;
      controller_sdp_ready_ = true;
      controller_sdp_cv_.notify_one();
    });

    controller_->onIceCandidate([this](const std::string& candidate) {
      controller_ice_candidates_.push_back(candidate);
    });

    controller_->onIceGatheringStateChange([this](IceGatheringState state) {
      controller_ice_state_ = state;
      if (state == IceGatheringState::kComplete) {
        controller_ice_complete_ = true;
        controller_ice_cv_.notify_one();
      }
    });

    // Controlled callbacks
    controlled_->onLocalDescription([this](const std::string& sdp) {
      controlled_local_sdp_ = sdp;
      controlled_sdp_ready_ = true;
      controlled_sdp_cv_.notify_one();
    });

    controlled_->onIceCandidate([this](const std::string& candidate) {
      controlled_ice_candidates_.push_back(candidate);
    });

    controlled_->onIceGatheringStateChange([this](IceGatheringState state) {
      controlled_ice_state_ = state;
      if (state == IceGatheringState::kComplete) {
        controlled_ice_complete_ = true;
        controlled_ice_cv_.notify_one();
      }
    });

    // Data receive callbacks
    controller_->setDataCallback([this](const std::vector<uint8_t>& data) {
      controller_received_data_ = data;
      controller_data_received_ = true;
      controller_data_cv_.notify_one();
    });

    controlled_->setDataCallback([this](const std::vector<uint8_t>& data) {
      controlled_received_data_ = data;
      controlled_data_received_ = true;
      controlled_data_cv_.notify_one();
    });

    // State change callbacks
    controller_->setStateCallback([this](DataChannelState state) {
      controller_state_ = state;
    });

    controlled_->setStateCallback([this](DataChannelState state) {
      controlled_state_ = state;
    });
  }

  /**
   * @brief Establish full connection between controller and controlled
   */
  bool establishConnection(int timeout_ms = 10000) {
    // Step 1: Controller creates offer
    auto offer_result = controller_->createOffer();
    if (!offer_result) {
      std::cout << "Failed to create offer: " << offer_result.error().message << std::endl;
      return false;
    }

    std::string offer = offer_result.value();
    std::cout << "Created offer (" << offer.size() << " bytes)" << std::endl;

    // Step 2: Controlled receives offer and creates answer
    auto set_offer_result = controlled_->setRemoteDescription(offer, SdpType::kOffer);
    if (!set_offer_result) {
      std::cout << "Failed to set remote offer: " << set_offer_result.error().message << std::endl;
      return false;
    }

    auto answer_result = controlled_->createAnswer();
    if (!answer_result) {
      std::cout << "Failed to create answer: " << answer_result.error().message << std::endl;
      return false;
    }

    std::string answer = answer_result.value();
    std::cout << "Created answer (" << answer.size() << " bytes)" << std::endl;

    // Step 3: Controller receives answer
    auto set_answer_result = controller_->setRemoteDescription(answer, SdpType::kAnswer);
    if (!set_answer_result) {
      std::cout << "Failed to set remote answer: " << set_answer_result.error().message << std::endl;
      return false;
    }

    // Step 4: Exchange ICE candidates
    exchangeIceCandidates(timeout_ms);

    // Step 5: Wait for connection
    return waitForConnection(timeout_ms);
  }

  /**
   * @brief Exchange ICE candidates between peers
   */
  void exchangeIceCandidates(int timeout_ms = 5000) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    // Wait for ICE gathering to complete on both sides
    {
      std::unique_lock<std::mutex> lock(controller_ice_mutex_);
      controller_ice_cv_.wait_until(lock, deadline, [this] {
        return controller_ice_complete_.load();
      });
    }

    {
      std::unique_lock<std::mutex> lock(controlled_ice_mutex_);
      controlled_ice_cv_.wait_until(lock, deadline, [this] {
        return controlled_ice_complete_.load();
      });
    }

    // Exchange candidates
    for (const auto& cand : controller_ice_candidates_) {
      if (!cand.empty()) {
        [[maybe_unused]] auto result = controlled_->addIceCandidate(cand);
      }
    }

    for (const auto& cand : controlled_ice_candidates_) {
      if (!cand.empty()) {
        [[maybe_unused]] auto result = controller_->addIceCandidate(cand);
      }
    }

    std::cout << "Exchanged " << controller_ice_candidates_.size()
              << " + " << controlled_ice_candidates_.size()
              << " ICE candidates" << std::endl;
  }

  /**
   * @brief Wait for DataChannel to reach open state
   */
  bool waitForConnection(int timeout_ms = 10000) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline) {
      if (controller_->isConnected() && controlled_->isConnected()) {
        std::cout << "Both DataChannels connected" << std::endl;
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "Connection timeout - Controller state: "
              << static_cast<int>(controller_state_)
              << ", Controlled state: "
              << static_cast<int>(controlled_state_) << std::endl;
    return false;
  }

  /**
   * @brief Wait for data to be received
   */
  bool waitForDataReceived(std::atomic<bool>& flag,
                           std::condition_variable& cv,
                           std::mutex& mutex,
                           int timeout_ms = 5000) {
    std::unique_lock<std::mutex> lock(mutex);
    return cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                       [&flag] { return flag.load(); });
  }

  DataChannel* controller_{nullptr};
  DataChannel* controlled_{nullptr};
  DataChannelConfig config_;

  // Signaling state
  std::string controller_local_sdp_;
  std::string controlled_local_sdp_;
  std::vector<std::string> controller_ice_candidates_;
  std::vector<std::string> controlled_ice_candidates_;
  std::atomic<bool> controller_sdp_ready_{false};
  std::atomic<bool> controlled_sdp_ready_{false};
  std::atomic<bool> controller_ice_complete_{false};
  std::atomic<bool> controlled_ice_complete_{false};
  IceGatheringState controller_ice_state_{IceGatheringState::kNew};
  IceGatheringState controlled_ice_state_{IceGatheringState::kNew};

  // Data receive state
  std::atomic<bool> controller_data_received_{false};
  std::atomic<bool> controlled_data_received_{false};
  std::vector<uint8_t> controller_received_data_;
  std::vector<uint8_t> controlled_received_data_;

  // Connection state
  DataChannelState controller_state_{DataChannelState::kNew};
  DataChannelState controlled_state_{DataChannelState::kNew};

  // Synchronization
  std::mutex controller_sdp_mutex_;
  std::mutex controlled_sdp_mutex_;
  std::mutex controller_ice_mutex_;
  std::mutex controlled_ice_mutex_;
  std::mutex controller_data_mutex_;
  std::mutex controlled_data_mutex_;
  std::condition_variable controller_sdp_cv_;
  std::condition_variable controlled_sdp_cv_;
  std::condition_variable controller_ice_cv_;
  std::condition_variable controlled_ice_cv_;
  std::condition_variable controller_data_cv_;
  std::condition_variable controlled_data_cv_;
};

TEST_F(TransportTest, CreateOffer) {
  auto result = controller_->createOffer();

  ASSERT_TRUE(result) << "Offer creation should succeed";
  EXPECT_FALSE(result.value().empty()) << "Offer SDP should not be empty";

  std::cout << "Offer created (" << result.value().size() << " bytes)" << std::endl;
}

TEST_F(TransportTest, CreateAnswer) {
  // First create offer from controller
  auto offer_result = controller_->createOffer();
  ASSERT_TRUE(offer_result) << "Offer creation should succeed";

  // Set remote offer on controlled
  auto set_offer_result = controlled_->setRemoteDescription(offer_result.value(), SdpType::kOffer);
  ASSERT_TRUE(set_offer_result) << "Set remote offer should succeed";

  // Create answer on controlled
  auto answer_result = controlled_->createAnswer();

  ASSERT_TRUE(answer_result) << "Answer creation should succeed";
  EXPECT_FALSE(answer_result.value().empty()) << "Answer SDP should not be empty";

  std::cout << "Answer created (" << answer_result.value().size() << " bytes)" << std::endl;
}

TEST_F(TransportTest, EstablishConnection) {
  bool connected = establishConnection();

  ASSERT_TRUE(connected) << "Connection should be established";
  EXPECT_TRUE(controller_->isConnected()) << "Controller should be connected";
  EXPECT_TRUE(controlled_->isConnected()) << "Controlled should be connected";
}

TEST_F(TransportTest, ConnectionStateTransitions) {
  EXPECT_EQ(controller_->getState(), DataChannelState::kNew)
    << "Initial state should be kNew";

  // After connection establishment, should reach kOpen
  bool connected = establishConnection();
  ASSERT_TRUE(connected);

  EXPECT_EQ(controller_->getState(), DataChannelState::kOpen)
    << "State should be kOpen when connected";
  EXPECT_EQ(controlled_->getState(), DataChannelState::kOpen)
    << "State should be kOpen when connected";
}

TEST_F(TransportTest, SendBinaryData) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  // Send binary data from controller to controlled
  std::vector<uint8_t> test_data = {1, 2, 3, 4, 5, 0xFF, 0xFE, 0xFD};
  auto send_result = controller_->send(test_data);

  ASSERT_TRUE(send_result) << "Send should succeed";

  // Wait for data to be received
  bool received = waitForDataReceived(controlled_data_received_, controlled_data_cv_,
                                      controlled_data_mutex_, 3000);

  ASSERT_TRUE(received) << "Data should be received";
  EXPECT_EQ(controlled_received_data_, test_data)
    << "Received data should match sent data";
}

TEST_F(TransportTest, SendTextData) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  // Send text data
  std::string test_text = "Hello, WebRTC!";
  auto send_result = controller_->send(test_text);

  ASSERT_TRUE(send_result) << "Send should succeed";

  // Wait for data to be received
  bool received = waitForDataReceived(controlled_data_received_, controlled_data_cv_,
                                      controlled_data_mutex_, 3000);

  ASSERT_TRUE(received) << "Data should be received";

  std::string received_text(controlled_received_data_.begin(),
                            controlled_received_data_.end());
  EXPECT_EQ(received_text, test_text) << "Received text should match sent text";
}

TEST_F(TransportTest, BidirectionalCommunication) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  // Send from controller to controlled
  std::vector<uint8_t> data1 = {1, 2, 3};
  [[maybe_unused]] auto send1 = controller_->send(data1);
  ASSERT_TRUE(send1);

  bool received1 = waitForDataReceived(controlled_data_received_, controlled_data_cv_,
                                       controlled_data_mutex_, 3000);
  ASSERT_TRUE(received1);
  EXPECT_EQ(controlled_received_data_, data1);

  // Reset flag
  controller_data_received_ = false;

  // Send from controlled to controller
  std::vector<uint8_t> data2 = {4, 5, 6};
  [[maybe_unused]] auto send2 = controlled_->send(data2);

  bool received2 = waitForDataReceived(controller_data_received_, controller_data_cv_,
                                       controller_data_mutex_, 3000);
  ASSERT_TRUE(received2);
  EXPECT_EQ(controller_received_data_, data2);
}

TEST_F(TransportTest, LargeMessageTransmission) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  // Send large message (256KB - within libdatachannel limits)
  const size_t kLargeSize = 256 * 1024;
  std::vector<uint8_t> large_data(kLargeSize);
  for (size_t i = 0; i < kLargeSize; ++i) {
    large_data[i] = static_cast<uint8_t>(i % 256);
  }

  auto send_result = controller_->send(large_data);
  ASSERT_TRUE(send_result) << "Send should succeed for large data";

  // Wait for data to be received
  bool received = waitForDataReceived(controlled_data_received_, controlled_data_cv_,
                                      controlled_data_mutex_, 10000);

  ASSERT_TRUE(received) << "Large data should be received";
  EXPECT_EQ(controlled_received_data_.size(), kLargeSize)
    << "Received size should match sent size";

  // Verify data integrity (sample check)
  const size_t kCheckInterval = 1000;
  for (size_t i = 0; i < kLargeSize; i += kCheckInterval) {
    EXPECT_EQ(controlled_received_data_[i], large_data[i])
      << "Data mismatch at position " << i;
  }

  std::cout << "Successfully transmitted 256KB data" << std::endl;
}

TEST_F(TransportTest, RapidMessageTransmission) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  // Send multiple rapid messages
  const int kMessageCount = 100;
  std::atomic<int> received_count{0};

  controlled_->setDataCallback([&](const std::vector<uint8_t>& /* data */) {
    received_count++;
  });

  for (int i = 0; i < kMessageCount; ++i) {
    std::vector<uint8_t> data = {static_cast<uint8_t>(i & 0xFF)};
    [[maybe_unused]] auto send_result = controller_->send(data);
    ASSERT_TRUE(send_result) << "Send " << i << " should succeed";
  }

  // Wait for all messages to be received
  auto start = std::chrono::steady_clock::now();
  while (received_count.load() < kMessageCount) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
    if (elapsed > 10000) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_EQ(received_count.load(), kMessageCount)
    << "All messages should be received";

  std::cout << "Received " << received_count.load() << "/" << kMessageCount
            << " messages" << std::endl;
}

TEST_F(TransportTest, SendWhenNotConnected) {
  // Don't establish connection
  std::vector<uint8_t> data = {1, 2, 3};

  auto send_result = controller_->send(data);

  EXPECT_FALSE(send_result) << "Send should fail when not connected";
  EXPECT_EQ(send_result.error().type, ErrorType::kNetworkError);
}

TEST_F(TransportTest, Disconnect) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  EXPECT_TRUE(controller_->isConnected()) << "Should be connected before disconnect";
  EXPECT_TRUE(controlled_->isConnected()) << "Should be connected before disconnect";

  // Disconnect controller
  controller_->disconnect();

  EXPECT_FALSE(controller_->isConnected()) << "Controller should not be connected after disconnect";
  EXPECT_EQ(controller_->getState(), DataChannelState::kClosed)
    << "Controller state should be kClosed";
}

TEST_F(TransportTest, EmptyMessageTransmission) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  // Try to send empty data
  std::vector<uint8_t> empty_data;
  auto send_result = controller_->send(empty_data);

  EXPECT_FALSE(send_result) << "Send empty data should fail";
  EXPECT_EQ(send_result.error().type, ErrorType::kNetworkError);
}

TEST_F(TransportTest, EmptyTextMessageTransmission) {
  ASSERT_TRUE(establishConnection()) << "Connection should be established";

  // Try to send empty text
  std::string empty_text;
  auto send_result = controller_->send(empty_text);

  EXPECT_FALSE(send_result) << "Send empty text should fail";
  EXPECT_EQ(send_result.error().type, ErrorType::kNetworkError);
}

TEST_F(TransportTest, ConnectionLatencyMeasurement) {
  const int kIterationCount = 5;
  std::vector<int64_t> latencies_ms;

  for (int i = 0; i < kIterationCount; ++i) {
    // Recreate DataChannels
    delete controller_;
    delete controlled_;
    controller_ = new DataChannel(config_);
    controlled_ = new DataChannel(config_);
    setupSignaling();

    // Measure connection time
    auto start = std::chrono::steady_clock::now();

    bool connected = establishConnection(15000);

    auto end = std::chrono::steady_clock::now();
    auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start).count();

    if (connected) {
      latencies_ms.push_back(latency_ms);
      std::cout << "Connection " << (i + 1) << ": " << latency_ms << "ms" << std::endl;
    } else {
      std::cout << "Connection " << (i + 1) << ": failed" << std::endl;
    }
  }

  ASSERT_GT(latencies_ms.size(), 0) << "At least one connection should succeed";

  // Calculate statistics
  int64_t total = 0;
  int64_t max_latency = 0;
  for (auto lat : latencies_ms) {
    total += lat;
    if (lat > max_latency) {
      max_latency = lat;
    }
  }

  double avg_ms = static_cast<double>(total) / latencies_ms.size();

  std::cout << "Connection latency - Avg: " << avg_ms
            << "ms, Max: " << max_latency << "ms" << std::endl;

  // Connection should complete in reasonable time
  EXPECT_LT(avg_ms, 3000.0) << "Average connection time should be < 3s";
  EXPECT_LT(max_latency, 5000) << "Max connection time should be < 5s";
}

TEST_F(TransportTest, MultipleConnectionsInSequence) {
  const int kConnectionCount = 3;
  std::atomic<int> success_count{0};

  for (int i = 0; i < kConnectionCount; ++i) {
    // Clean up previous connection
    if (i > 0) {
      controller_->disconnect();
      controlled_->disconnect();
      delete controller_;
      delete controlled_;
      controller_ = new DataChannel(config_);
      controlled_ = new DataChannel(config_);
      setupSignaling();
    }

    // Establish connection
    if (establishConnection()) {
      success_count++;

      // Send a test message
      std::vector<uint8_t> data = {static_cast<uint8_t>(i)};
      [[maybe_unused]] auto send_result = controller_->send(data);
      if (send_result) {
        // Reset receive flag
        controlled_data_received_ = false;
        bool received = waitForDataReceived(controlled_data_received_,
                                            controlled_data_cv_,
                                            controlled_data_mutex_, 3000);
        if (received && controlled_received_data_ == data) {
          success_count++;
        }
      }
    }
  }

  EXPECT_GE(success_count.load(), kConnectionCount)
    << "All connections and messages should succeed";

  std::cout << "Successful connections: " << success_count.load() << "/"
            << (kConnectionCount * 2) << std::endl;
}

} // namespace screensdk
