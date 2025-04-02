#include "web_transport.h"
#include <iostream>

#include "h264_depacketizer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

int main() {
    web_transport::Client client("https://opengit.ai/counter");
    
    client.setCACertBundleFile("/etc/ssl/certs/ca-certificates.crt");
    client.setCACertDir("/etc/ssl/certs");
    client.connect();

    client.onSessionOpen([](void* session_ptr) {
        auto* session = static_cast<web_transport::ClientSession*>(session_ptr);
        std::cout << "Session opened!" << std::endl;
    
        auto depacketizer = std::make_shared<H264Depacketizer>();


        // Open the output file in binary mode to write H264 data.
        auto output_file = std::make_shared<std::ofstream>("output.h264", std::ios::binary);
        if (!output_file->is_open()) {
            std::cerr << "Failed to open output.h264 for writing" << std::endl;
            return;
        }

        // Register the datagram read callback, capturing the depacketizer and the output file.
        session->onDatagramRead([depacketizer, output_file](std::vector<uint8_t> data) {
            std::vector<uint8_t> h264_frame;
                
            // Receive RTP packet
            if (depacketizer->Depacketize(data, &h264_frame)) {
                std::cout << "Decoded H264 frame size: " << h264_frame.size() << std::endl;
            } else {
                std::cerr << "Decoding failed" << std::endl;
            }
    
            //std::cout << "Received H264 encoded frame size: " << h264_payload.size() << std::endl;
            
            // Write the H264 payload to the output file.
            output_file->write(reinterpret_cast<const char*>(h264_frame.data()), h264_frame.size());
            if (output_file->fail()) {
                std::cerr << "Failed to write to output.h264" << std::endl;
            }
        });
    });
    
    client.onSessionError([](const std::string& error) {
        std::cout << "Session error: " << error << std::endl;
    });
    
    client.runEventLoop(); // This should block and process events
    
    return 0;
}