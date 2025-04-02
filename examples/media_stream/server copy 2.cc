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


    // Set up a session callback. Capture udp_socket by value so it is available in the lambda.
    server.onSession([](void* session_ptr, const std::string& path) {
        auto* session = static_cast<web_transport::ServerSession*>(session_ptr);
        std::cout << "New session on path: " << path << std::endl;

        return true;
    });

    server.onBidirectionalStream([udp_socket](void* session_ptr, void* stream_ptr, const std::string& path) {

        auto* stream = static_cast<web_transport::ServerStream*>(stream_ptr);
        auto* session = static_cast<web_transport::ServerSession*>(session_ptr);
        std::cout << "New bidirectional stream on path: " << path << std::endl;
        

        // Use setInterval to poll the UDP socket every ~16ms (~60 FPS)
        stream->setInterval(16, [stream, udp_socket]() {
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
                stream->send(buffer);
            }

            // If no data is available, recvfrom will return -1 (or SOCKET_ERROR on Windows)
            // with an error indicating that the operation would block.
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