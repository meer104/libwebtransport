#include "web_transport.h"
#include <iostream>

int main() {
    web_transport::Server server("0.0.0.0", 443);
    
    server.setCertFile("/root/libwebtransport/ssls/fullchain.pem");
    server.setKeyFile("/root/libwebtransport/ssls/privkey.pem");
    
    server.onSession([](void* session_ptr, const std::string& path) {

        auto* session = static_cast<web_transport::ServerSession*>(session_ptr);
        std::cout << "New session on path: " << path << std::endl;

        // Echo data back to client
        session->onDatagramRead([session](const std::vector<uint8_t>& data) {
            std::cout << "onDatagramRead: " << data.data() << std::endl;
        });
        
        
        return true;
    });
    
    server.onBidirectionalStream([](void* session_ptr, void* stream_ptr, const std::string& path) {

    });
    
    server.initialize();
    server.listen();
    
    return 0;
}