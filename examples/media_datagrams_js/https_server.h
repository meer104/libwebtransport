// httpserver.hpp

#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <string>
#include <thread>

class HttpServer {
public:
    HttpServer(const std::string& certPath, const std::string& keyPath,
               const std::string& bindAddress, int port);
    ~HttpServer();

    void start();
    void stop();

private:
    void setupRoutes();
    void run();

    httplib::SSLServer server_;
    std::string bindAddress_;
    int port_;
    std::thread serverThread_;
    bool running_;
};