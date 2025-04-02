#include "web_transport.h"
#include <iostream>

int main() {
    web_transport::Client client("https://opengit.ai/counter");
    
    client.setCACertBundleFile("/etc/ssl/certs/ca-certificates.crt");
    client.setCACertDir("/etc/ssl/certs");
    
    client.onSessionOpen([](void* session_ptr) {
        auto* session = static_cast<web_transport::ClientSession*>(session_ptr);
        std::cout << "Session opened!" << std::endl;
        
        // Send a datagram
        session->sendDatagram({1, 2, 3, 4});
    });
    
    client.onSessionError([](const std::string& error) {
        std::cout << "Session error: " << error << std::endl;
    });
    
    client.connect();
    client.runEventLoop();
    
    return 0;
}