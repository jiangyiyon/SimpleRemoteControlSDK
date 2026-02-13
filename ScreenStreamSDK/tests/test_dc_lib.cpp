#include <rtc/rtc.h>
#include <rtc/peerconnection.hpp>
#include <rtc/global.hpp>
#include <iostream>

int main() {
    try {
        std::cout << "Initializing libdatachannel..." << std::endl;
        rtc::InitLogger(rtc::LogLevel::Debug);
        rtc::Preload();
        std::cout << "  Initialized!" << std::endl;

        std::cout << "Creating PeerConnection..." << std::endl;
        rtc::Configuration config;
        auto pc = std::make_shared<rtc::PeerConnection>(config);
        std::cout << "  PeerConnection created!" << std::endl;

        std::cout << "Creating DataChannel..." << std::endl;
        rtc::DataChannelInit init;
        init.protocol = "test-protocol";
        auto dc = pc->createDataChannel("test-channel", init);
        std::cout << "  DataChannel created!" << std::endl;

        std::cout << "Creating offer..." << std::endl;
        auto offer = pc->createOffer();
        std::cout << "  Offer created! Length: " << offer.generateSdp().length() << std::endl;

        std::cout << "Success!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
