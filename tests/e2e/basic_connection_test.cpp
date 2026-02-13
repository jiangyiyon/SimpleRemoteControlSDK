#include <gtest/gtest.h>

#include <thread>

#include "e2e_test_helper.h"

namespace screensdk {

class BasicConnectionTest : public ::testing::Test {
protected:
  void SetUp() override {
    controller_ = std::make_unique<MockController>();
    remote_host_ = std::make_unique<MockRemoteHost>();

    ASSERT_TRUE(controller_->initialize());
    ASSERT_TRUE(remote_host_->initialize(0));
  }

  void TearDown() override {
    if (controller_) {
      controller_->cleanup();
      controller_.reset();
    }

    if (remote_host_) {
      remote_host_->cleanup();
      remote_host_.reset();
    }
  }

  std::unique_ptr<MockController> controller_;
  std::unique_ptr<MockRemoteHost> remote_host_;
};

TEST_F(BasicConnectionTest, InitializeBothSides) {
  EXPECT_NE(controller_->getDataChannel(), nullptr);
  EXPECT_NE(remote_host_->getDataChannel(), nullptr);
}

TEST_F(BasicConnectionTest, EstablishConnection) {
  bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);

  EXPECT_TRUE(connected);
  EXPECT_TRUE(controller_->getDataChannel()->isConnected());
  EXPECT_TRUE(remote_host_->getDataChannel()->isConnected());
}

TEST_F(BasicConnectionTest, CreateOfferAndAnswer) {
  DataChannel* remote_dc = remote_host_->getDataChannel();
  DataChannel* controller_dc = controller_->getDataChannel();

  auto offer_result = remote_dc->createOffer();
  EXPECT_TRUE(offer_result);
  EXPECT_FALSE(offer_result.value().empty());

  auto answer_result = controller_dc->createAnswer();
  EXPECT_TRUE(answer_result);
  EXPECT_FALSE(answer_result.value().empty());
}

TEST_F(BasicConnectionTest, ExchangeIceCandidates) {
  bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);

  ASSERT_TRUE(connected);

  // Allow some time for ICE connection
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  EXPECT_EQ(controller_->getDataChannel()->getState(), DataChannelState::kOpen);
  EXPECT_EQ(remote_host_->getDataChannel()->getState(), DataChannelState::kOpen);
}

TEST_F(BasicConnectionTest, DisconnectAndReconnect) {
  bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);

  ASSERT_TRUE(connected);

  // Disconnect
  controller_->getDataChannel()->disconnect();
  remote_host_->getDataChannel()->disconnect();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  EXPECT_FALSE(controller_->getDataChannel()->isConnected());
  EXPECT_FALSE(remote_host_->getDataChannel()->isConnected());
}

TEST_F(BasicConnectionTest, SendTextMessage) {
  bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);

  ASSERT_TRUE(connected);

  bool message_received = false;
  std::string received_message;

  controller_->getDataChannel()->setDataCallback([&](const std::vector<uint8_t>& data) {
    received_message = std::string(data.begin(), data.end());
    message_received = true;
  });

  // Send message from remote host to controller
  std::string test_message = "Hello from remote host!";
  remote_host_->getDataChannel()->send(test_message);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  EXPECT_TRUE(message_received);
  EXPECT_EQ(received_message, test_message);
}

TEST_F(BasicConnectionTest, BidirectionalCommunication) {
  bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);

  ASSERT_TRUE(connected);

  bool remote_to_controller_received = false;
  bool controller_to_remote_received = false;
  std::string received_from_remote;
  std::string received_from_controller;

  controller_->getDataChannel()->setDataCallback([&](const std::vector<uint8_t>& data) {
    received_from_remote = std::string(data.begin(), data.end());
    remote_to_controller_received = true;
  });

  remote_host_->getDataChannel()->setDataCallback([&](const std::vector<uint8_t>& data) {
    received_from_controller = std::string(data.begin(), data.end());
    controller_to_remote_received = true;
  });

  // Send messages in both directions
  remote_host_->getDataChannel()->send("Remote -> Controller");
  controller_->getDataChannel()->send("Controller -> Remote");

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  EXPECT_TRUE(remote_to_controller_received);
  EXPECT_TRUE(controller_to_remote_received);
  EXPECT_EQ(received_from_remote, "Remote -> Controller");
  EXPECT_EQ(received_from_controller, "Controller -> Remote");
}

TEST_F(BasicConnectionTest, LargeMessage) {
  bool connected = ConnectionHelper::establishConnection(*remote_host_, *controller_);

  ASSERT_TRUE(connected);

  bool message_received = false;
  std::vector<uint8_t> received_data;

  controller_->getDataChannel()->setDataCallback([&](const std::vector<uint8_t>& data) {
    received_data = data;
    message_received = true;
  });

  // Send 200KB message (DataChannel typically has 256KB limit)
  std::vector<uint8_t> large_message(200 * 1024);
  for (size_t i = 0; i < large_message.size(); ++i) {
    large_message[i] = static_cast<uint8_t>(i % 256);
  }

  remote_host_->getDataChannel()->send(large_message);

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  EXPECT_TRUE(message_received);
  EXPECT_EQ(received_data.size(), large_message.size());
}

TEST_F(BasicConnectionTest, ConnectionTimeout) {
  // Don't establish connection
  controller_->waitForConnection(1000);

  EXPECT_FALSE(controller_->getDataChannel()->isConnected());
}

} // namespace screensdk
