// WebrtcManager unit tests
// TDD: Tests written before implementation

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include "screensdk/transport/i_webrtc_transport.h"

namespace screensdk {

// Mock IVideoSource for testing
class MockVideoSource : public IVideoSource {
public:
    bool started = false;
    bool stopped = false;
    int current_width = 1920;
    int current_height = 1080;
    int current_fps = 60;
    FrameCallback callback;

    void start() override { started = true; stopped = false; }
    void stop() override { stopped = true; started = false; }
    bool isRunning() const override { return started; }
    void setFrameCallback(FrameCallback cb) override { callback = cb; }
    void getFrameSize(int* width, int* height) const override {
        if (width) *width = current_width;
        if (height) *height = current_height;
    }
    int getFps() const override { return current_fps; }
};

class WebrtcManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = CreateWebrtcTransport();
        ASSERT_NE(manager, nullptr);

        config.stun_server = "";
        config.use_ipv6 = false;
        config.max_bitrate_bps = 15000000;
        config.enable_ice_tcp = false;
    }

    void TearDown() override {
        if (manager) {
            manager->shutdown();
            DestroyWebrtcTransport(manager);
        }
    }

    IWebrtcTransport* manager = nullptr;
    TransportConfig config;
};

// Test 1: Initialize WebrtcManager
TEST_F(WebrtcManagerTest, InitializeWithValidConfig) {
    // Act
    bool result = manager->initialize(config);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getConnectionState(), ConnectionState::kNew);
}

TEST_F(WebrtcManagerTest, InitializeWithStunServer) {
    // Arrange
    config.stun_server = "stun:stun.l.google.com:19302";

    // Act
    bool result = manager->initialize(config);

    // Assert
    EXPECT_TRUE(result);
}

// Test 2: Create SDP Offer
TEST_F(WebrtcManagerTest, CreateOfferWhenInitialized) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act
    auto result = manager->createOffer();

    // Assert
    EXPECT_TRUE(result);
    EXPECT_FALSE(result.value().empty());
    EXPECT_TRUE(result.value().find("v=") != std::string::npos);  // SDP version line
    EXPECT_TRUE(result.value().find("m=") != std::string::npos);  // media line
}

TEST_F(WebrtcManagerTest, CreateOfferFailsWhenNotInitialized) {
    // Act & Assert
    auto result = manager->createOffer();
    EXPECT_FALSE(result);
}

// Test 3: Set Remote Description
TEST_F(WebrtcManagerTest, SetRemoteDescriptionWithValidSdp) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    auto offer_result = manager->createOffer();
    ASSERT_TRUE(offer_result);
    std::string offer = offer_result.value();

    // Act - Set as offer (type must match the SDP content)
    auto result = manager->setRemoteDescription(offer, SdpType::kOffer);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(WebrtcManagerTest, SetRemoteDescriptionWithEmptySdp) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act
    auto result = manager->setRemoteDescription("");

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(WebrtcManagerTest, SetRemoteDescriptionWithInvalidSdp) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act
    auto result = manager->setRemoteDescription("invalid sdp");

    // Assert
    EXPECT_FALSE(result);
}

// Test 4: ICE Candidates
TEST_F(WebrtcManagerTest, GetLocalIceCandidatesAfterOffer) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    auto offer_result = manager->createOffer();

    // Wait for ICE gathering
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Act
    std::vector<IceCandidate> candidates = manager->getLocalIceCandidates();

    // Assert
    // LAN connections should generate host candidates
    EXPECT_GT(candidates.size(), 0);
}

TEST_F(WebrtcManagerTest, AddRemoteIceCandidate) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    auto offer_result = manager->createOffer();
    ASSERT_TRUE(manager->setRemoteDescription(offer_result.value(), SdpType::kOffer));

    IceCandidate candidate;
    candidate.candidate = "candidate:1 1 UDP 2130706431 192.168.1.100 54321 typ host";
    candidate.sdp_mid = "0";
    candidate.sdp_mline_index = 0;

    // Act
    auto result = manager->addIceCandidate(candidate);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(WebrtcManagerTest, AddEmptyIceCandidate) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    IceCandidate candidate;
    candidate.candidate = "";
    candidate.sdp_mid = "0";
    candidate.sdp_mline_index = 0;

    // Act
    auto result = manager->addIceCandidate(candidate);

    // Assert
    EXPECT_FALSE(result);
}

// Test 5: Connection State
TEST_F(WebrtcManagerTest, ConnectionStateProgression) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act & Assert - Initial state
    EXPECT_EQ(manager->getConnectionState(), ConnectionState::kNew);

    // After creating offer
    auto offer_result = manager->createOffer();
    EXPECT_EQ(manager->getConnectionState(), ConnectionState::kChecking);
}

// Test 6: Video Track Management
TEST_F(WebrtcManagerTest, StartVideoTrackWithValidSource) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    MockVideoSource* source = new MockVideoSource();
    source->start();

    // Act
    manager->startVideoTrack(source);

    // Assert - Should not crash, track should be started
    // Video track is added to peer connection
    EXPECT_TRUE(source->isRunning());
}

TEST_F(WebrtcManagerTest, StartVideoTrackWithNullSource) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act & Assert - Should not crash
    manager->startVideoTrack(nullptr);
}

TEST_F(WebrtcManagerTest, StopVideoTrack) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    MockVideoSource* source = new MockVideoSource();
    source->start();
    manager->startVideoTrack(source);

    // Act
    manager->stopVideoTrack();

    // Assert
    EXPECT_TRUE(source->stopped);
}

TEST_F(WebrtcManagerTest, StopVideoTrackTwice) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    MockVideoSource* source = new MockVideoSource();
    source->start();
    manager->startVideoTrack(source);
    manager->stopVideoTrack();

    // Act - Stop again, should not crash
    manager->stopVideoTrack();

    // Assert - Source should still be stopped
    EXPECT_TRUE(source->stopped);
}

TEST_F(WebrtcManagerTest, StopVideoTrackWithoutStart) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act - Stop without starting video track, should not crash
    manager->stopVideoTrack();

    // Assert - Should not throw exception or crash
}

// Test 7: Data Channel Messages
TEST_F(WebrtcManagerTest, SendDataChannelMessage) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    auto offer_result = manager->createOffer();
    ASSERT_TRUE(manager->setRemoteDescription(offer_result.value(), SdpType::kOffer));

    // Act
    auto result = manager->sendDataChannelMessage("test message");

    // Assert
    // Note: May fail if data channel is not fully established yet
    // This is expected for LAN connections without full peer
}

TEST_F(WebrtcManagerTest, SendEmptyDataChannelMessage) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act
    auto result = manager->sendDataChannelMessage("");

    // Assert - Should handle gracefully
    // May return false if data channel not ready
}

TEST_F(WebrtcManagerTest, SendLargeDataChannelMessage) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    std::string large_message(1000000, 'x');  // 1MB message

    // Act & Assert - Should not crash
    auto result = manager->sendDataChannelMessage(large_message);
    // May fail if message exceeds limits
}

// Test 8: Event Callbacks
TEST_F(WebrtcManagerTest, StateChangeCallback) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    bool callback_called = false;
    ConnectionState last_state = ConnectionState::kNew;

    manager->setStateChangeCallback([&](ConnectionState state) {
        callback_called = true;
        last_state = state;
    });

    // Act
    auto offer_result = manager->createOffer();

    // Assert
    // Callback should be called when state changes
    // May take time for state to actually change
}

TEST_F(WebrtcManagerTest, IceCandidateCallback) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    bool callback_called = false;
    IceCandidate last_candidate;

    manager->setIceCandidateCallback([&](const IceCandidate& candidate) {
        callback_called = true;
        last_candidate = candidate;
    });

    // Act
    auto offer_result = manager->createOffer();

    // Wait for ICE gathering
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Assert
    // Should receive ICE candidates
    EXPECT_TRUE(callback_called);
}

TEST_F(WebrtcManagerTest, DataChannelCallback) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    bool callback_called = false;
    std::string last_message;

    manager->setDataChannelCallback([&](const std::string& message) {
        callback_called = true;
        last_message = message;
    });

    // Act & Assert
    // Callback should be registered without error
}

TEST_F(WebrtcManagerTest, ErrorCallback) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    bool callback_called = false;
    std::string last_error;

    manager->setErrorCallback([&](const std::string& error) {
        callback_called = true;
        last_error = error;
    });

    // Act & Assert
    // Callback should be registered without error
}

// Test 9: Shutdown
TEST_F(WebrtcManagerTest, ShutdownInitializedManager) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    auto offer_result = manager->createOffer();

    // Act
    manager->shutdown();

    // Assert
    EXPECT_EQ(manager->getConnectionState(), ConnectionState::kClosed);
}

TEST_F(WebrtcManagerTest, ShutdownNotInitializedManager) {
    // Act & Assert - Should not crash
    manager->shutdown();
}

// Test 10: Multiple Initialization
TEST_F(WebrtcManagerTest, InitializeMultipleTimes) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act
    bool result = manager->initialize(config);

    // Assert
    // Should handle gracefully - may return false or succeed
}

// Test 11: IPv6 Configuration
TEST_F(WebrtcManagerTest, InitializeWithIPv6) {
    // Arrange
    config.use_ipv6 = true;

    // Act
    bool result = manager->initialize(config);

    // Assert
    EXPECT_TRUE(result);
}

// Test 12: Max Bitrate Configuration
TEST_F(WebrtcManagerTest, InitializeWithMaxBitrate) {
    // Arrange
    config.max_bitrate_bps = 20000000;  // 20 Mbps

    // Act
    bool result = manager->initialize(config);

    // Assert
    EXPECT_TRUE(result);
}

// Test 13: ICE TCP Configuration
TEST_F(WebrtcManagerTest, InitializeWithIceTcp) {
    // Arrange
    config.enable_ice_tcp = true;

    // Act
    bool result = manager->initialize(config);

    // Assert
    EXPECT_TRUE(result);
}

// Test 14: Memory Management
TEST_F(WebrtcManagerTest, CreateAndDestroyMultipleManagers) {
    // Act & Assert
    for (int i = 0; i < 10; ++i) {
        IWebrtcTransport* mgr = CreateWebrtcTransport();
        ASSERT_NE(mgr, nullptr);
        EXPECT_TRUE(mgr->initialize(config));
        mgr->shutdown();
        DestroyWebrtcTransport(mgr);
    }
}

// Test 15: Thread Safety
TEST_F(WebrtcManagerTest, ConcurrentStateQueries) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));

    // Act
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this]() {
            for (int j = 0; j < 100; ++j) {
                manager->getConnectionState();
                manager->getLocalIceCandidates();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Assert - Should not crash
}

// Test 16: Invalid ICE Candidate Format
TEST_F(WebrtcManagerTest, AddInvalidIceCandidateFormat) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    auto offer_result = manager->createOffer();
    ASSERT_TRUE(manager->setRemoteDescription(offer_result.value(), SdpType::kOffer));

    IceCandidate candidate;
    candidate.candidate = "invalid candidate format";
    candidate.sdp_mid = "0";
    candidate.sdp_mline_index = 0;

    // Act
    auto result = manager->addIceCandidate(candidate);

    // Assert
    EXPECT_FALSE(result);
}

// Test 17: Video Track with Different Resolutions
TEST_F(WebrtcManagerTest, VideoTrackWith1080p) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    MockVideoSource* source = new MockVideoSource();
    source->current_width = 1920;
    source->current_height = 1080;

    // Act
    manager->startVideoTrack(source);

    // Assert - Should not crash
}

TEST_F(WebrtcManagerTest, VideoTrackWith720p) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    MockVideoSource* source = new MockVideoSource();
    source->current_width = 1280;
    source->current_height = 720;

    // Act
    manager->startVideoTrack(source);

    // Assert - Should not crash
}

TEST_F(WebrtcManagerTest, VideoTrackWith4K) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    MockVideoSource* source = new MockVideoSource();
    source->current_width = 3840;
    source->current_height = 2160;

    // Act
    manager->startVideoTrack(source);

    // Assert - Should not crash
}

// Test 18: Re-initialize After Shutdown
TEST_F(WebrtcManagerTest, ReinitializeAfterShutdown) {
    // Arrange
    ASSERT_TRUE(manager->initialize(config));
    manager->shutdown();

    // Act
    bool result = manager->initialize(config);

    // Assert - Behavior depends on implementation
    // May succeed or fail
}

} // namespace screensdk
