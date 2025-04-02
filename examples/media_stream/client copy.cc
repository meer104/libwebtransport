#include "web_transport.h"
#include <iostream>

#include "rtp_packet.h"
#include "h264_packet.h"
#include "h264_rtp_packetizer.h"
#include "h264_rtp_depacketizer.h"


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
    
        // Create a shared pointer for the H264 depacketizer.
        auto h264_depacketizer = std::make_shared<H264Depacketizer>();

        // Open the output file in binary mode to write H264 data.
        auto output_file = std::make_shared<std::ofstream>("output.h264", std::ios::binary);
        if (!output_file->is_open()) {
            std::cerr << "Failed to open output.h264 for writing" << std::endl;
            return;
        }

        // Register the datagram read callback, capturing the depacketizer and the output file.
        session->onDatagramRead([h264_depacketizer, output_file](std::vector<uint8_t> data) {
            RTPPacket rtp_packet;
            if (!rtp_packet.Unmarshal(data.data(), data.size())) {
                std::cerr << "Failed to parse RTP packet" << std::endl;
                return;
            }
    
            std::cout << "Received rtp packet size: " << rtp_packet.GetPayloadSize() << std::endl;
    
            std::vector<uint8_t> h264_payload;
            if (!h264_depacketizer->Unmarshal(rtp_packet.GetPayload(), rtp_packet.GetPayloadSize(), &h264_payload)) {
                std::cerr << "Failed to depacketize H264 payload (possibly due to lost RTP packets)" << std::endl;
                return;
            }
    
            std::cout << "Received h264 encoded frame size: " << h264_payload.size() << std::endl;
            
            // Write the H264 payload to the output file.
            output_file->write(reinterpret_cast<const char*>(h264_payload.data()), h264_payload.size());
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