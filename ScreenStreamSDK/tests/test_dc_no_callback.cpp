#include "screensdk/transport/data_channel.h"
#include <iostream>

int main() {
    try {
        std::cout << "Test: Create offer without callbacks" << std::endl;
        {
            screensdk::DataChannelConfig config;
            config.label = "simple-channel";
            screensdk::DataChannel dc(config);

            std::cout << "  Calling createOffer..." << std::endl;
            auto result = dc.createOffer();
            std::cout << "  createOffer returned!" << std::endl;

            if (result) {
                std::cout << "  Success! SDP length: " << result.value().length() << std::endl;
                std::cout << "  SDP preview: " << result.value().substr(0, 100) << "..." << std::endl;
            } else {
                const auto& err = result.error();
                std::cout << "  Failed! Error code: " << err.code << ", message: " << err.message << std::endl;
            }
        }

        std::cout << "  DataChannel destroyed!" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
