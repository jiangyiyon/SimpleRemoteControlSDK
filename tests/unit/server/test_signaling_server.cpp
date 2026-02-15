/**
 * @file test_signaling_server.cpp
 * @brief Unit tests for SignalingServer
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "screensdk/server/signaling_server.h"

namespace screensdk::server::test {

class SignalingServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<SignalingServer>();
    }

    void TearDown() override {
        if (server_ && server_->isRunning()) {
            server_->stop();
        }
    }

protected:
    std::unique_ptr<SignalingServer> server_;
};

TEST_F(SignalingServerTest, StartAndStopServer) {
    auto result = server_->start(9000);
    EXPECT_TRUE(result);
    EXPECT_TRUE(server_->isRunning());

    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}

TEST_F(SignalingServerTest, StartWithInvalidPort) {
    auto result = server_->start(-1);
    EXPECT_FALSE(result);
    EXPECT_FALSE(server_->isRunning());

    result = server_->start(0);
    EXPECT_FALSE(result);

    result = server_->start(65536);
    EXPECT_FALSE(result);
}

TEST_F(SignalingServerTest, StartWithValidPort) {
    auto result = server_->start(9001);
    EXPECT_TRUE(result);
    EXPECT_TRUE(server_->isRunning());

    server_->stop();
}

TEST_F(SignalingServerTest, StartAlreadyRunningServer) {
    auto result1 = server_->start(9002);
    EXPECT_TRUE(result1);
    EXPECT_TRUE(server_->isRunning());

    auto result2 = server_->start(9003);
    EXPECT_FALSE(result2);

    server_->stop();
}

TEST_F(SignalingServerTest, GetClientCountWhenNoClients) {
    auto result = server_->start(9003);
    EXPECT_TRUE(result);

    EXPECT_EQ(server_->getClientCount(), 0);

    server_->stop();
}

TEST_F(SignalingServerTest, StopNotRunningServer) {
    // Should not crash when stopping a non-running server
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}

TEST_F(SignalingServerTest, MultipleStartStopCycles) {
    for (int i = 0; i < 3; ++i) {
        auto result = server_->start(9004 + i);
        EXPECT_TRUE(result);
        EXPECT_TRUE(server_->isRunning());

        server_->stop();
        EXPECT_FALSE(server_->isRunning());

        // Small delay to allow port cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TEST_F(SignalingServerTest, StartWithMinPort) {
    auto result = server_->start(1);
    EXPECT_TRUE(result);
    EXPECT_TRUE(server_->isRunning());

    server_->stop();
}

TEST_F(SignalingServerTest, StartWithMaxPort) {
    auto result = server_->start(65535);
    EXPECT_TRUE(result);
    EXPECT_TRUE(server_->isRunning());

    server_->stop();
}

} // namespace screensdk::server::test
