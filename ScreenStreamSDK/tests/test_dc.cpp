#include "screensdk/transport/data_channel.h"
#include <iostream>

int main() {
    try {
        screensdk::DataChannelConfig config;
        config.label = "test-channel";
        config.protocol = "test-protocol";

        std::cout << "Creating DataChannel..." << std::endl;
        screensdk::DataChannel dc(config);
        std::cout << "DataChannel created!" << std::endl;

        std::cout << "Creating offer..." << std::endl;
        auto result = dc.createOffer();
        std::cout << "createOffer() returned" << std::endl;

        if (result) {
            std::cout << "Success! SDP:\n" << result.value() << std::endl;
        } else {
            const auto& err = result.error();
            std::cout << "Failed! Error code: " << err.code << ", message: " << err.message << std::endl;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
