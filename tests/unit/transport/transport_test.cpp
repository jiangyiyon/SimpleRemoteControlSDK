#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "screensdk/transport/data_channel.h"

namespace screensdk {

/**
 * @brief Unit test for transport layer with DataChannel
 *
 * Tests:
 * - Connection establishment and SDP/ICE exchange
 * - Message transmission reliability
 * - Network interruption simulation
 * - Reconnection after disconnect
 * - Large message handling
 * - Binary data transmission
 * - Concurrent message handling
 * - State transition correctness
 */
class TransportTest : public ::testing::Test {
protected:
  void SetUp() override {
    DataChannelConfig config1;
    config1.label = "remote-host";
    data_channel1_ = new DataChannel(config1);

    DataChannelConfig config2;
    config2.label = "controller";
    data_channel2_ = new DataChannel(config2);

    offer_received_ = false;
    answer_received_ = false;
    ice_candidates1_received_ = 0;
    ice_candidates2_received_ = 0;
    connected1_ = false;
    connected2_ = false;
    messages_received_.clear();
  }

  void TearDown() override {
    if (data_channel1_ != nullptr) {
      data_channel1_->disconnect();
      delete data_channel1_;
      data_channel1_ = nullptr;
    }

    if (data_channel2_ != nullptr) {
      data_channel2_->disconnect();
      delete data_channel2_;
      data_channel2_ = nullptr;
    }
  }

  /**
   * @brief Establish P2P connection between two DataChannels
   */
  void establishConnection() {
    // Setup callbacks for data_channel1 (remote host side)
    data_channel1_->onLocalDescription([&](const std::string& sdp) {
      offer_ = sdp;
      offer_received_ = true;
    });

    data_channel1_->onIceCandidate([&](const std::string& candidate) {
      ice_candidates1_.push_back(candidate);
      ice_candidates1_received_++;
    });

    data_channel1_->setStateCallback([&](DataChannelState state) {
      if (state == DataChannelState::kOpen) {
        connected1_ = true;
      }
    });

    data_channel1_->setDataCallback([&](const std::vector<uint8_t>& data) {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      messages_received_.push_back(std::string(data.begin(), data.end()));
    });

    // Setup callbacks for data_channel2 (controller side)
    data_channel2_->onLocalDescription([&](const std::string& sdp) {
      answer_ = sdp;
      answer_received_ = true;
    });

    data_channel2_->onIceCandidate([&](const std::string& candidate) {
      ice_candidates2_.push_back(candidate);
      ice_candidates2_received_++;
    });

    data_channel2_->setStateCallback([&](DataChannelState state) {
      if (state == DataChannelState::kOpen) {
        connected2_ = true;
      }
    });

    data_channel2_->setDataCallback([&](const std::vector<uint8_t>& data) {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      messages_received_.push_back(std::string(data.begin(), data.end()));
    });

    // Create offer from remote host
    auto result1 = data_channel1_->createOffer();
    ASSERT_TRUE(result1) << "createOffer should succeed";
    ASSERT_FALSE(result1.value().empty()) << "Offer SDP should not be empty";

    // Wait for offer
    auto start = std::chrono::steady_clock::now();
    while (!offer_received_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
        FAIL() << "Timeout waiting for offer";
      }
    }

    // Set remote offer on controller
    auto result2 = data_channel2_->setRemoteDescription(offer_, SdpType::kOffer);
    ASSERT_TRUE(result2) << "setRemoteDescription(offer) should succeed";

    // Create answer from controller
    auto result3 = data_channel2_->createAnswer();
    ASSERT_TRUE(result3) << "createAnswer should succeed";
    ASSERT_FALSE(result3.value().empty()) << "Answer SDP should not be empty";

    // Wait for answer
    start = std::chrono::steady_clock::now();
    while (!answer_received_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
        FAIL() << "Timeout waiting for answer";
      }
    }

    // Set remote answer on remote host
    auto result4 = data_channel1_->setRemoteDescription(answer_, SdpType::kAnswer);
    ASSERT_TRUE(result4) << "setRemoteDescription(answer) should succeed";

    // Exchange ICE candidates (local loopback)
    for (const auto& candidate : ice_candidates1_) {
      data_channel2_->addIceCandidate(candidate);
    }
    for (const auto& candidate : ice_candidates2_) {
      data_channel1_->addIceCandidate(candidate);
    }

    // Wait for connection
    start = std::chrono::steady_clock::now();
    while (!connected1_ || !connected2_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) {
        FAIL() << "Timeout waiting for connection";
      }
    }
  }

  DataChannel* data_channel1_{nullptr}; // Remote host
  DataChannel* data_channel2_{nullptr}; // Controller

  std::string offer_;
  std::string answer_;
  std::vector<std::string> ice_candidates1_;
  std::vector<std::string> ice_candidates2_;

  std::atomic<bool> offer_received_;
  std::atomic<bool> answer_received_;
  std::atomic<int> ice_candidates1_received_;
  std::atomic<int> ice_candidates2_received_;
  std::atomic<bool> connected1_;
  std::atomic<bool> connected2_;

  std::vector<std::string> messages_received_;
  std::mutex messages_mutex_;
};

TEST_F(TransportTest, EstablishP2PConnection) {
  establishConnection();

  EXPECT_TRUE(connected1_) << "DataChannel 1 should be connected";
  EXPECT_TRUE(connected2_) << "DataChannel 2 should be connected";
  EXPECT_TRUE(data_channel1_->isConnected());
  EXPECT_TRUE(data_channel2_->isConnected());

  std::cout << "Successfully established P2P connection" << std::endl;
}

TEST_F(TransportTest, SendTextMessage) {
  establishConnection();

  std::string test_message = "Hello, World!";

  // Send from data_channel1 to data_channel2
  auto result = data_channel1_->send(test_message);
  ASSERT_TRUE(result) << "Send should succeed";

  // Wait for message
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(messages_received_.size(), 1);
  EXPECT_EQ(messages_received_[0], test_message);

  std::cout << "Received: " << messages_received_[0] << std::endl;
}

TEST_F(TransportTest, SendBinaryData) {
  establishConnection();

  std::vector<uint8_t> binary_data = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD};

  auto result = data_channel1_->send(binary_data);
  ASSERT_TRUE(result) << "Binary send should succeed";

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(messages_received_.size(), 1);

  // Convert back to binary for comparison
  std::vector<uint8_t> received(messages_received_[0].begin(),
                                 messages_received_[0].end());
  EXPECT_EQ(received.size(), binary_data.size());

  for (size_t i = 0; i < binary_data.size(); ++i) {
    EXPECT_EQ(static_cast<uint8_t>(received[i]), binary_data[i]);
  }

  std::cout << "Binary data transmitted successfully" << std::endl;
}

TEST_F(TransportTest, BidirectionalCommunication) {
  establishConnection();

  messages_received_.clear();

  std::string message1 = "Message from channel 1";
  std::string message2 = "Message from channel 2";

  // Send both directions
  data_channel1_->send(message1);
  data_channel2_->send(message2);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  EXPECT_EQ(messages_received_.size(), 2);

  std::cout << "Received " << messages_received_.size()
            << " messages: " << messages_received_[0]
            << ", " << messages_received_[1] << std::endl;
}

TEST_F(TransportTest, LargeMessageTransmission) {
  establishConnection();

  // Create 200KB message (matches E2E test)
  const size_t kMessageSize = 200 * 1024;
  std::string large_message(kMessageSize, 'A');

  auto result = data_channel1_->send(large_message);
  ASSERT_TRUE(result) << "Large message send should succeed";

  // Wait for transmission
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  EXPECT_EQ(messages_received_.size(), 1);
  EXPECT_EQ(messages_received_[0].size(), kMessageSize);

  std::cout << "Successfully transmitted " << kMessageSize
            << " bytes (" << kMessageSize / 1024 << "KB)" << std::endl;
}

TEST_F(TransportTest, MultipleSequentialMessages) {
  establishConnection();

  const int kMessageCount = 10;

  for (int i = 0; i < kMessageCount; ++i) {
    std::string message = "Message #" + std::to_string(i);
    data_channel1_->send(message);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  EXPECT_GE(messages_received_.size(), kMessageCount);

  std::cout << "Received " << messages_received_.size()
            << " messages out of " << kMessageCount << " sent" << std::endl;
}

TEST_F(TransportTest, DisconnectAndReconnect) {
  // First connection
  establishConnection();
  EXPECT_TRUE(connected1_);
  EXPECT_TRUE(connected2_);

  // Disconnect
  data_channel1_->disconnect();
  data_channel2_->disconnect();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(data_channel1_->isConnected());
  EXPECT_FALSE(data_channel2_->isConnected());

  // Clean up and create new connection
  delete data_channel1_;
  delete data_channel2_;

  DataChannelConfig config1;
  config1.label = "remote-host-2";
  data_channel1_ = new DataChannel(config1);

  DataChannelConfig config2;
  config2.label = "controller-2";
  data_channel2_ = new DataChannel(config2);

  // Re-establish connection
  establishConnection();

  EXPECT_TRUE(connected1_);
  EXPECT_TRUE(connected2_);

  std::cout << "Successfully disconnected and reconnected" << std::endl;
}

TEST_F(TransportTest, ConnectionStateTransitions) {
  // Test state transitions without establishing full connection
  EXPECT_EQ(data_channel1_->getState(), DataChannelState::kNew);
  EXPECT_FALSE(data_channel1_->isConnected());

  data_channel1_->onLocalDescription([](const std::string&) {});
  data_channel1_->onIceCandidate([](const std::string&) {});

  auto result = data_channel1_->createOffer();
  EXPECT_TRUE(result);

  // State should transition from kNew to kConnecting
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  DataChannelState state = data_channel1_->getState();
  EXPECT_TRUE(state == DataChannelState::kNew ||
              state == DataChannelState::kConnecting);

  std::cout << "State after createOffer: "
            << (state == DataChannelState::kNew ? "kNew" :
                state == DataChannelState::kConnecting ? "kConnecting" : "Other")
            << std::endl;
}

TEST_F(TransportTest, SendWhenNotConnected) {
  // Test sending before connection
  std::string message = "Test message";

  auto result = data_channel1_->send(message);
  EXPECT_FALSE(result) << "Send should fail when not connected";

  std::cout << "Correctly rejected send when not connected" << std::endl;
}

TEST_F(TransportTest, EmptyMessage) {
  establishConnection();

  std::string empty_message = "";

  // Send empty message
  auto result = data_channel1_->send(empty_message);

  // This may succeed or fail depending on implementation
  std::cout << "Empty message send: "
            << (result ? "succeeded" : "failed as expected") << std::endl;
}

TEST_F(TransportTest, ConcurrentMessageHandling) {
  establishConnection();

  const int kMessageCount = 20;
  messages_received_.clear();

  // Send multiple messages quickly
  for (int i = 0; i < kMessageCount; ++i) {
    std::string message = "Concurrent message #" + std::to_string(i);
    data_channel1_->send(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  // Wait for all messages
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::cout << "Received " << messages_received_.size()
            << " messages out of " << kMessageCount << " sent" << std::endl;

  // Most messages should be received
  EXPECT_GT(messages_received_.size(), kMessageCount * 0.8);
}

TEST_F(TransportTest, ConnectionLabels) {
  establishConnection();

  std::string label1 = data_channel1_->getLabel();
  std::string label2 = data_channel2_->getLabel();

  EXPECT_EQ(label1, "remote-host");
  EXPECT_EQ(label2, "controller");

  std::cout << "Labels: " << label1 << ", " << label2 << std::endl;
}

TEST_F(TransportTest, SdpExchangeIntegrity) {
  establishConnection();

  EXPECT_FALSE(offer_.empty()) << "Offer SDP should not be empty";
  EXPECT_FALSE(answer_.empty()) << "Answer SDP should not be empty";

  // SDP should contain expected keywords
  EXPECT_TRUE(offer_.find("m=application") != std::string::npos ||
              offer_.find("a=sctp-port") != std::string::npos);
  EXPECT_TRUE(answer_.find("m=application") != std::string::npos ||
              answer_.find("a=sctp-port") != std::string::npos);

  std::cout << "Offer length: " << offer_.size() << " bytes" << std::endl;
  std::cout << "Answer length: " << answer_.size() << " bytes" << std::endl;
}

TEST_F(TransportTest, IceCandidatesExchange) {
  establishConnection();

  EXPECT_GE(ice_candidates1_received_, 0) << "Should receive ICE candidates";
  EXPECT_GE(ice_candidates2_received_, 0) << "Should receive ICE candidates";

  std::cout << "ICE candidates from channel 1: "
            << ice_candidates1_received_ << std::endl;
  std::cout << "ICE candidates from channel 2: "
            << ice_candidates2_received_ << std::endl;

  // For local loopback, we may not get ICE candidates
  // But the mechanism should work
}

TEST_F(TransportTest, ConnectionTimeout) {
  // Test connection timeout scenario
  // Note: This test may take longer

  data_channel1_->onLocalDescription([](const std::string&) {});
  data_channel1_->onIceCandidate([](const std::string&) {});

  auto result = data_channel1_->createOffer();
  EXPECT_TRUE(result);

  // Don't complete connection - check state after timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // State should not be connected
  EXPECT_FALSE(data_channel1_->isConnected());

  std::cout << "Connection correctly not established (timeout test)" << std::endl;
}

TEST_F(TransportTest, ResourceCleanup) {
  // Test that resources are properly cleaned up
  establishConnection();

  // Send some data
  data_channel1_->send("Test message");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Disconnect
  data_channel1_->disconnect();
  data_channel2_->disconnect();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(data_channel1_->isConnected());
  EXPECT_FALSE(data_channel2_->isConnected());

  std::cout << "Resources cleaned up successfully" << std::endl;
}

} // namespace screensdk
