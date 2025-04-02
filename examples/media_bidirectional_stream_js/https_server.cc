// httpserver.cpp

#include "https_server.h"
#include <iostream>

HttpServer::HttpServer(const std::string& certPath, const std::string& keyPath,
                       const std::string& bindAddress, int port)
    : server_(certPath.c_str(), keyPath.c_str()),
      bindAddress_(bindAddress),
      port_(port),
      running_(false) {
    setupRoutes();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (!running_) {
        running_ = true;
        serverThread_ = std::thread(&HttpServer::run, this);
    }
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        server_.stop();
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }

}

void HttpServer::setupRoutes() {
    server_.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        res.set_file_content("/root/libwebtransport/examples/media_bidirectional_stream_js/index.html", "text/html");
    });
}

void HttpServer::run() {
    std::cout << "HTTP Server running on port " << port_ << "..." << std::endl;
    server_.listen(bindAddress_.c_str(), port_);
}