#include "screensdk/transport/data_channel.h"
#include <iostream>

int main() {
    try {
        std::cout << "Test 1: Basic construction" << std::endl;
        {
            screensdk::DataChannelConfig config;
            config.label = "test-channel";
            screensdk::DataChannel dc(config);
            std::cout << "  DataChannel created successfully!" << std::endl;
        }
        std::cout << "  DataChannel destroyed!" << std::endl;

        std::cout << "Test 2: Create offer with minimal config" << std::endl;
        {
            screensdk::DataChannelConfig config;
            config.label = "simple-channel";
            screensdk::DataChannel dc(config);

            auto result = dc.createOffer();
            if (result) {
                std::cout << "  Success! SDP length: " << result.value().length() << std::endl;
            } else {
                const auto& err = result.error();
                std::cout << "  Failed! Error code: " << err.code << ", message: " << err.message << std::endl;
            }
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
