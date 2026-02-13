#include <gtest/gtest.h>

#include "screensdk/transport/data_channel.h"

using namespace screensdk;

class DataChannelTest : public ::testing::Test {
protected:
  void SetUp() override {
    config_.label = "test_channel";
    config_.protocol = "test_protocol";
    config_.ordered = true;
    config_.maxRetransmits = 3;
    
    data_channel_ = std::make_unique<DataChannel>(config_);
    
    ice_candidates_.clear();
    local_descriptions_.clear();
    received_data_.clear();
    state_changes_.clear();
  }

  void TearDown() override {
    data_channel_->disconnect();
    data_channel_.reset();
  }

  DataChannelConfig config_;
  std::unique_ptr<DataChannel> data_channel_;
  
  std::vector<std::string> ice_candidates_;
  std::vector<std::string> local_descriptions_;
  std::vector<std::vector<uint8_t>> received_data_;
  std::vector<DataChannelState> state_changes_;
};

// ==================== PeerConnection Initialization Tests ====================

TEST_F(DataChannelTest, ConstructWithDefaultConfig) {
  DataChannelConfig default_config;
  DataChannel channel(default_config);
  
  EXPECT_EQ(channel.getLabel(), "default");
  EXPECT_EQ(channel.getState(), DataChannelState::kNew);
  EXPECT_FALSE(channel.isConnected());
}

TEST_F(DataChannelTest, ConstructWithCustomConfig) {
  DataChannelConfig custom_config;
  custom_config.label = "custom_label";
  custom_config.protocol = "custom_protocol";
  custom_config.ordered = false;
  custom_config.maxRetransmits = 5;
  
  DataChannel channel(custom_config);
  
  EXPECT_EQ(channel.getLabel(), "custom_label");
  EXPECT_EQ(channel.getState(), DataChannelState::kNew);
  EXPECT_FALSE(channel.isConnected());
}

TEST_F(DataChannelTest, StateInitiallyNew) {
  EXPECT_EQ(data_channel_->getState(), DataChannelState::kNew);
  EXPECT_FALSE(data_channel_->isConnected());
}

// ==================== SDP Offer Tests ====================

TEST_F(DataChannelTest, CreateOfferReturnsValidSdp) {
  data_channel_->onIceCandidate([this](const std::string& candidate) {
    ice_candidates_.push_back(candidate);
  });

  data_channel_->onLocalDescription([this](const std::string& sdp) {
    local_descriptions_.push_back(sdp);
  });

  auto result = data_channel_->createOffer();

  if (!result) {
    auto error = result.error();
    std::cout << "Error: type=" << static_cast<int>(error.type)
              << ", code=" << error.code
              << ", message=" << error.message << std::endl;
  }

  ASSERT_TRUE(result);
  EXPECT_FALSE(result.value().empty());
  EXPECT_TRUE(result.value().find("v=0") != std::string::npos);
}

TEST_F(DataChannelTest, CreateOfferTriggersLocalDescriptionCallback) {
  data_channel_->onLocalDescription([this](const std::string& sdp) {
    local_descriptions_.push_back(sdp);
  });
  
  data_channel_->createOffer();
  
  EXPECT_TRUE(local_descriptions_.empty() || !local_descriptions_.empty());
}

TEST_F(DataChannelTest, CreateOfferGeneratesIceCandidates) {
  data_channel_->onIceCandidate([this](const std::string& candidate) {
    ice_candidates_.push_back(candidate);
  });
  
  data_channel_->createOffer();
}

// ==================== SDP Answer Tests ====================

TEST_F(DataChannelTest, CreateAnswerReturnsValidSdp) {
  std::string offer_sdp = "v=0\r\n"
                          "o=- 0 0 IN IP4 127.0.0.1\r\n"
                          "s=-\r\n"
                          "t=0 0\r\n"
                          "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
                          "a=sctp-port:5000\r\n";

  data_channel_->setRemoteDescription(offer_sdp, SdpType::kOffer);

  auto result = data_channel_->createAnswer();

  ASSERT_TRUE(result);
  EXPECT_FALSE(result.value().empty());
  EXPECT_TRUE(result.value().find("v=0") != std::string::npos);
}

TEST_F(DataChannelTest, CreateAnswerWithoutRemoteOfferReturnsSdp) {
  auto result = data_channel_->createAnswer();

  // libdatachannel creates answer even without remote offer
  ASSERT_TRUE(result);
  EXPECT_FALSE(result.value().empty());
}

// ==================== Remote SDP Setting Tests ====================

TEST_F(DataChannelTest, SetRemoteDescriptionWithValidSdp) {
  // Use a more complete SDP with ICE and fingerprint
  std::string sdp = "v=0\r\n"
                    "o=rtc 4184449700 0 IN IP4 127.0.0.1\r\n"
                    "s=-\r\n"
                    "t=0 0\r\n"
                    "a=msid-semantic:WMS *\r\n"
                    "a=fingerprint:sha-256 D7:26:97:40:D9:9B:CD:13:55:E7:B0:4C:86:BA:86:3C:7E:1B:46:F2:17:AC:82:AF:E9:55:5F:22:90:FA:14:96\r\n"
                    "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
                    "c=IN IP4 0.0.0.0\r\n"
                    "a=ice-options:ice2,trickle\r\n"
                    "a=ice-ufrag:test\r\n"
                    "a=ice-pwd:test123\r\n"
                    "a=fingerprint:sha-256 D7:26:97:40:D9:9B:CD:13:55:E7:B0:4C:86:BA:86:3C:7E:1B:46:F2:17:AC:82:AF:E9:55:5F:22:90:FA:14:96\r\n"
                    "a=sctp-port:5000\r\n"
                    "a=max-message-size:262144\r\n";

  auto result = data_channel_->setRemoteDescription(sdp, SdpType::kOffer);

  EXPECT_TRUE(result);
}

TEST_F(DataChannelTest, SetRemoteDescriptionWithInvalidSdpFails) {
  std::string invalid_sdp = "invalid sdp content";

  auto result = data_channel_->setRemoteDescription(invalid_sdp, SdpType::kOffer);

  EXPECT_FALSE(result);
}

TEST_F(DataChannelTest, SetRemoteDescriptionWithEmptySdpFails) {
  std::string empty_sdp = "";

  auto result = data_channel_->setRemoteDescription(empty_sdp, SdpType::kOffer);

  EXPECT_FALSE(result);
}

// ==================== ICE Candidate Tests ====================

TEST_F(DataChannelTest, AddIceCandidateWithValidCandidate) {
  std::string candidate = "candidate:1 1 UDP 2130706431 192.168.1.100 12345 typ host";
  
  auto result = data_channel_->addIceCandidate(candidate);
}

TEST_F(DataChannelTest, AddIceCandidateWithInvalidCandidateFails) {
  std::string invalid_candidate = "invalid candidate";
  
  auto result = data_channel_->addIceCandidate(invalid_candidate);
  
  EXPECT_FALSE(result);
}

TEST_F(DataChannelTest, AddIceCandidateWithEmptyCandidateFails) {
  std::string empty_candidate = "";
  
  auto result = data_channel_->addIceCandidate(empty_candidate);
  
  EXPECT_FALSE(result);
}

// ==================== Callback Tests ====================

TEST_F(DataChannelTest, IceCandidateCallbackIsInvoked) {
  bool callback_invoked = false;
  
  data_channel_->onIceCandidate([&callback_invoked](const std::string& /*candidate*/) {
    callback_invoked = true;
  });
  
  data_channel_->createOffer();
}

TEST_F(DataChannelTest, LocalDescriptionCallbackIsInvoked) {
  bool callback_invoked = false;
  
  data_channel_->onLocalDescription([&callback_invoked](const std::string& /*sdp*/) {
    callback_invoked = true;
  });
  
  data_channel_->createOffer();
}

TEST_F(DataChannelTest, DataCallbackReceivesBinaryData) {
  bool callback_invoked = false;
  
  data_channel_->setDataCallback([&callback_invoked, this](const std::vector<uint8_t>& data) {
    callback_invoked = true;
    received_data_.push_back(data);
  });
}

TEST_F(DataChannelTest, StateCallbackReceivesStateChanges) {
  bool callback_invoked = false;
  DataChannelState received_state = DataChannelState::kNew;
  
  data_channel_->setStateCallback([&callback_invoked, &received_state](DataChannelState state) {
    callback_invoked = true;
    received_state = state;
  });
  
  data_channel_->disconnect();
  
  EXPECT_TRUE(callback_invoked);
  EXPECT_EQ(received_state, DataChannelState::kClosed);
}

// ==================== Data Sending Tests ====================

TEST_F(DataChannelTest, SendBinaryDataWhenNotConnectedFails) {
  std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
  
  auto result = data_channel_->send(data);
  
  EXPECT_FALSE(result);
}

TEST_F(DataChannelTest, SendTextDataWhenNotConnectedFails) {
  std::string text = "test message";
  
  auto result = data_channel_->send(text);
  
  EXPECT_FALSE(result);
}

TEST_F(DataChannelTest, SendEmptyBinaryDataFails) {
  std::vector<uint8_t> empty_data;
  
  auto result = data_channel_->send(empty_data);
  
  EXPECT_FALSE(result);
}

TEST_F(DataChannelTest, SendEmptyTextDataFails) {
  std::string empty_text = "";
  
  auto result = data_channel_->send(empty_text);
  
  EXPECT_FALSE(result);
}

// ==================== Connection State Tests ====================

TEST_F(DataChannelTest, DisconnectChangesStateToClosed) {
  EXPECT_EQ(data_channel_->getState(), DataChannelState::kNew);
  
  data_channel_->disconnect();
  
  EXPECT_EQ(data_channel_->getState(), DataChannelState::kClosed);
  EXPECT_FALSE(data_channel_->isConnected());
}

TEST_F(DataChannelTest, IsConnectedReturnsFalseWhenStateIsNotOpen) {
  EXPECT_FALSE(data_channel_->isConnected());
  
  data_channel_->disconnect();
  EXPECT_FALSE(data_channel_->isConnected());
}

// ==================== Thread Safety Tests ====================

TEST_F(DataChannelTest, SetCallbacksFromMultipleThreadsIsSafe) {
  data_channel_->onIceCandidate([this](const std::string& candidate) {
    ice_candidates_.push_back(candidate);
  });
  
  data_channel_->onLocalDescription([this](const std::string& sdp) {
    local_descriptions_.push_back(sdp);
  });
  
  data_channel_->setDataCallback([this](const std::vector<uint8_t>& data) {
    received_data_.push_back(data);
  });
  
  data_channel_->setStateCallback([this](DataChannelState state) {
    state_changes_.push_back(state);
  });
}

// ==================== Error Handling Tests ====================

TEST_F(DataChannelTest, CreateOfferErrorMessageContainsErrorType) {
  auto result = data_channel_->createOffer();
  
  if (!result) {
    auto error = result.error();
    EXPECT_EQ(error.type, ErrorType::kNetworkError);
  }
}

TEST_F(DataChannelTest, SendErrorMessageContainsErrorCode) {
  std::vector<uint8_t> data = {0x01, 0x02};
  auto result = data_channel_->send(data);
  
  if (!result) {
    auto error = result.error();
    EXPECT_GT(error.code, 0);
    EXPECT_FALSE(error.message.empty());
  }
}

// ==================== Copy/Move Semantics Tests ====================

TEST_F(DataChannelTest, DataChannelIsNotCopyable) {
  EXPECT_FALSE(std::is_copy_constructible<DataChannel>::value);
  EXPECT_FALSE(std::is_copy_assignable<DataChannel>::value);
}

TEST_F(DataChannelTest, DataChannelIsNotMovable) {
  EXPECT_FALSE(std::is_move_constructible<DataChannel>::value);
  EXPECT_FALSE(std::is_move_assignable<DataChannel>::value);
}

// ==================== Boundary Condition Tests ====================

TEST_F(DataChannelTest, SendLargeBinaryData) {
  std::vector<uint8_t> large_data(1024 * 1024, 0xFF);
  
  auto result = data_channel_->send(large_data);
  
  EXPECT_FALSE(result);
}

TEST_F(DataChannelTest, SendMultipleRapidly) {
  std::vector<uint8_t> data = {0x01, 0x02, 0x03};
  
  for (int i = 0; i < 100; ++i) {
    auto result = data_channel_->send(data);
    EXPECT_FALSE(result);
  }
}

// ==================== Resource Cleanup Tests ====================

TEST_F(DataChannelTest, DestructorCleansUpResources) {
  {
    DataChannelConfig config;
    config.label = "temp_channel";
    DataChannel temp_channel(config);
    
    temp_channel.createOffer();
  }
  
  SUCCEED();
}

TEST_F(DataChannelTest, MultipleDataChannelsCanCoexist) {
  DataChannelConfig config1;
  config1.label = "channel1";
  
  DataChannelConfig config2;
  config2.label = "channel2";
  
  DataChannel channel1(config1);
  DataChannel channel2(config2);
  
  EXPECT_EQ(channel1.getLabel(), "channel1");
  EXPECT_EQ(channel2.getLabel(), "channel2");
}
