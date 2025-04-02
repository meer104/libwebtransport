#include <iostream>
#include <vector>
#include <string>

// Platform-specific includes and definitions
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

#include "web_transport.h" // Include your custom WebTransport header
#include "https_server.h" // Include your custom WebTransport header





class Frame {
public:
    enum class Type : uint8_t {
        RTP = 0,
        // Define other frame types as needed
    };

    Frame(Type type, uint64_t timestamp, const std::vector<uint8_t>& data)
        : type_(type), timestamp_(timestamp), data_(data) {}

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> framed_data;
        framed_data.reserve(1 + 8 + 4 + data_.size()); // Type(1) + Timestamp(8) + Size(4) + Data

        // Append Type as a single byte
        framed_data.push_back(static_cast<uint8_t>(type_));

        // Append Timestamp in big-endian (8 bytes)
        for (int i = 7; i >= 0; --i) {
            framed_data.push_back((timestamp_ >> (i * 8)) & 0xFF);
        }

        // Append Size in big-endian (4 bytes)
        uint32_t size = static_cast<uint32_t>(data_.size());
        for (int i = 3; i >= 0; --i) {
            framed_data.push_back((size >> (i * 8)) & 0xFF);
        }

        // Append the data
        framed_data.insert(framed_data.end(), data_.begin(), data_.end());

        return framed_data;
    }

private:
    Type type_;
    uint64_t timestamp_;
    std::vector<uint8_t> data_;
};



int main() {

    // ----- Initialize UDP Socket on Port 5000 -----
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed." << std::endl;
            return 1;
        }
    #endif

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "Failed to create UDP socket." << std::endl;
        return 1;
    }

    sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(5000);

    if (bind(udp_socket, reinterpret_cast<struct sockaddr*>(&udp_addr), sizeof(udp_addr)) < 0) {
        std::cerr << "Failed to bind UDP socket." << std::endl;
        return 1;
    }

    // Set the socket to non-blocking mode
    #ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(udp_socket, FIONBIO, &mode);
    #else
        int flags = fcntl(udp_socket, F_GETFL, 0);
        if (flags < 0) flags = 0;
        fcntl(udp_socket, F_SETFL, flags | O_NONBLOCK);
    #endif

    // ----- Initialize WebTransport Server -----
    web_transport::Server server("0.0.0.0", 443);
    server.setCertFile("/root/libwebtransport/ssls/fullchain.pem");
    server.setKeyFile("/root/libwebtransport/ssls/privkey.pem");


    // load https server for js client 
    auto httpServer = std::make_unique<HttpServer>(
        "/root/libwebtransport/ssls/fullchain.pem", 
        "/root/libwebtransport/ssls/privkey.pem", 
        "0.0.0.0", 
        443
    );
    httpServer->start();


    server.onSession([udp_socket](void* session_ptr, const std::string& path) {
        auto* session = static_cast<web_transport::ServerSession*>(session_ptr);
        std::cout << "New session on path: " << path << std::endl;
        return true;
    });

    server.onBidirectionalStream([udp_socket](void* session_ptr, void* stream_ptr, const std::string& path) {

        auto* stream = static_cast<web_transport::ServerStream*>(stream_ptr);
        auto* session = static_cast<web_transport::ServerSession*>(session_ptr);
        std::cout << "New bidirectional stream on path: " << path << std::endl;
        
        
               // 1 MS
        // don't miss rtp resend packets poll super fast !!!!
        stream->setInterval(1, [stream, udp_socket]() {
            // Prepare a buffer sized to typical MTU
            std::vector<uint8_t> buffer(65500);
            sockaddr_in src_addr;
            #ifdef _WIN32
                int addr_len = sizeof(src_addr);
            #else
                socklen_t addr_len = sizeof(src_addr);
            #endif

            // Attempt a non-blocking read from the UDP socket.
            int bytes_received = recvfrom(
                udp_socket,
                reinterpret_cast<char*>(buffer.data()),
                static_cast<int>(buffer.size()),
                0,
                reinterpret_cast<sockaddr*>(&src_addr),
                &addr_len
            );

            // Only process the data if we received any.
            if (bytes_received > 0) {

                std::cout << bytes_received << std::endl;


                // Resize !!!!!
                // Resize buffer to only include valid data.
                buffer.resize(bytes_received);

                // Obtain a current timestamp in milliseconds since epoch.
                auto now = std::chrono::system_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
                uint64_t timestamp = static_cast<uint64_t>(ms);

                // Create a frame with the received data.
                Frame frame(Frame::Type::RTP, timestamp, buffer);
                std::vector<uint8_t> serialized_frame = frame.serialize();


                stream->send(serialized_frame);
            }
        }); 
    });


    server.initialize();
    server.listen();

    // ----- Cleanup -----
    #ifdef _WIN32
        closesocket(udp_socket);
        WSACleanup();
    #else
        close(udp_socket);
    #endif

    return 0;
}