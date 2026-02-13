#include "screensdk/transport/data_channel.h"
#include <iostream>
#include <thread>

int main() {
    try {
        std::cout << "Test: Create offer with sleep" << std::endl;
        {
            screensdk::DataChannelConfig config;
            config.label = "simple-channel";
            screensdk::DataChannel dc(config);

            std::cout << "  DataChannel created!" << std::endl;
            
            std::cout << "  Sleeping for 100ms..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            std::cout << "  Calling createOffer..." << std::endl;
            auto result = dc.createOffer();
            std::cout << "  createOffer returned!" << std::endl;

            if (result) {
                std::cout << "  Success! SDP length: " << result.value().length() << std::endl;
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
