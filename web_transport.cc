#include "web_transport.h"

// Include internal headers
#include "web_transport_client.h"
#include "web_transport_client_session.h"
#include "web_transport_client_stream.h"
#include "web_transport_server.h"
#include "web_transport_server_session.h"
#include "web_transport_server_stream.h"

namespace web_transport {

//-----------------------------------------------------------------------------
// Client Implementation
//-----------------------------------------------------------------------------
Client::Client(const std::string& url) 
    : client_(std::make_unique<webtransport::Client>(url)) {
}

Client::~Client() = default;

void Client::setHeader(const std::string& key, const std::string& value) {
  client_->setHeader(key, value);
}

void Client::setPublicKeyFile(const std::string& file_path) {
  client_->setPublicKeyFile(file_path);
}

void Client::setCACertBundleFile(const std::string& file_path) {
  client_->setCACertBundleFile(file_path);
}

void Client::setCACertDir(const std::string& dir_path) {
  client_->setCACertDir(dir_path);
}

void Client::connect() {
  client_->connect();
}

void Client::disconnect() {
  client_->disconnect();
}

void Client::runEventLoop() {
  client_->runEventLoop();
}

void Client::onSessionOpen(std::function<void(void*)> callback) {
  session_callback_ = std::move(callback);
  
  client_->onSessionOpen([this](webtransport::ClientSession* internal_session) {
    if (session_callback_) {
      // Create a wrapper ClientSession and pass it to the user callback
      auto* session = new ClientSession(internal_session);
      session_callback_(session);
    }
  });
}

void Client::onBidirectionalStream(std::function<void(void*, void*)> callback) {
  bidi_stream_callback_ = std::move(callback);
  
  client_->onBidirectionalStream(
    [this](webtransport::ClientSession* internal_session, 
           webtransport::ClientBidirectionalStream* internal_stream) {
      if (bidi_stream_callback_) {
        // Create wrappers for both session and stream
        auto* session = new ClientSession(internal_session);
        auto* stream = new ClientStream(internal_stream);
        bidi_stream_callback_(session, stream);
      }
    });
}

void Client::onSessionError(std::function<void(const std::string&)> callback) {
  session_error_callback_ = std::move(callback);
  client_->onSessionError(session_error_callback_);
}

//-----------------------------------------------------------------------------
// ClientSession Implementation
//-----------------------------------------------------------------------------
ClientSession::ClientSession(void* session)
    : session_(static_cast<webtransport::ClientSession*>(session)) {
}

ClientSession::~ClientSession() = default;

void* ClientSession::createBidirectionalStream() {
  auto* stream = session_->createBidirectionalStream();
  return stream ? new ClientStream(stream) : nullptr;
}

void ClientSession::sendDatagram(const std::vector<uint8_t>& data) {
  session_->SendDatagram(data);
}

void ClientSession::setInterval(uint64_t interval_ms, std::function<void()> callback) {
  session_->setInterval(interval_ms, std::move(callback));
}

void ClientSession::onDatagramRead(std::function<void(std::vector<uint8_t>)> callback) {
  session_->onDatagramRead(std::move(callback));
}

void ClientSession::onBidirectionalStream(std::function<void(void*, void*)> callback) {
  bidi_stream_callback_ = std::move(callback);
  
  session_->onBidirectionalStream(
    [this](webtransport::ClientSession* internal_session, 
           webtransport::ClientBidirectionalStream* internal_stream) {
      if (bidi_stream_callback_) {
        // We already have a wrapper for the session (this)
        auto* stream = new ClientStream(internal_stream);
        bidi_stream_callback_(this, stream);
      }
    });
}

//-----------------------------------------------------------------------------
// ClientStream Implementation
//-----------------------------------------------------------------------------
ClientStream::ClientStream(void* stream)
    : stream_(static_cast<webtransport::ClientBidirectionalStream*>(stream)) {
}

ClientStream::~ClientStream() = default;

bool ClientStream::send(const std::vector<uint8_t>& data) {
  return stream_->Send(data);
}

void ClientStream::onStreamRead(std::function<void(std::vector<uint8_t>)> callback) {
  stream_->onStreamRead(std::move(callback));
}

void ClientStream::setInterval(uint64_t interval_ms, std::function<void()> callback) {
  stream_->setInterval(interval_ms, std::move(callback));
}

//-----------------------------------------------------------------------------
// Server Implementation
//-----------------------------------------------------------------------------
Server::Server(const std::string& host, uint16_t port)
    : server_(std::make_unique<webtransport::Server>(host, port)) {
}

Server::~Server() = default;

void Server::setCertFile(const std::string& cert_file) {
  server_->setCertFile(cert_file);
}

void Server::setKeyFile(const std::string& key_file) {
  server_->setKeyFile(key_file);
}

void Server::onSession(std::function<bool(void*, const std::string&)> callback) {
  session_callback_ = std::move(callback);
  
  server_->onSession([this](webtransport::ServerSession* internal_session, 
                           const std::string& path) {
    if (session_callback_) {
      auto* session = new ServerSession(internal_session);
      return session_callback_(session, path);
    }
    return true; // Accept by default if no callback set
  });
}

void Server::onUnidirectionalStream(
    std::function<void(void*, void*, const std::string&)> callback) {
  unidirectional_callback_ = std::move(callback);
  
  server_->onUnidirectionalStream(
    [this](webtransport::ServerSession* internal_session,
           webtransport::ServerUnidirectionalStream* internal_stream,
           const std::string& path) {
      if (unidirectional_callback_) {
        auto* session = new ServerSession(internal_session);
        // Cast to ServerStream* before storing as void*
        auto* stream = new ServerStream(static_cast<webtransport::ServerStream*>(internal_stream));
        unidirectional_callback_(session, stream, path);
      }
    });
}

void Server::onBidirectionalStream(
    std::function<void(void*, void*, const std::string&)> callback) {
  bidirectional_callback_ = std::move(callback);
  
  server_->onBidirectionalStream(
    [this](webtransport::ServerSession* internal_session,
           webtransport::ServerBidirectionalStream* internal_stream,
           const std::string& path) {
      if (bidirectional_callback_) {
        auto* session = new ServerSession(internal_session);
        // Cast to ServerStream* before storing as void*
        auto* stream = new ServerStream(static_cast<webtransport::ServerStream*>(internal_stream));
        bidirectional_callback_(session, stream, path);
      }
    });
}

void Server::initialize() {
  server_->InitializeServer();
}

void Server::listen() {
  server_->Listen();
}

//-----------------------------------------------------------------------------
// ServerSession Implementation
//-----------------------------------------------------------------------------
ServerSession::ServerSession(void* session)
    : session_(static_cast<webtransport::ServerSession*>(session)) {
}

ServerSession::~ServerSession() = default;

void ServerSession::sendDatagram(const std::vector<uint8_t>& data) {
  session_->SendDatagram(data);
}

void ServerSession::setInterval(uint64_t interval_ms, std::function<void()> callback) {
  session_->setInterval(interval_ms, std::move(callback));
}

void ServerSession::rejectSession(uint32_t error_code, const std::string& reason) {
  session_->RejectSession(error_code, reason);
}

void ServerSession::onDatagramRead(std::function<void(std::vector<uint8_t>)> callback) {
  session_->onDatagramRead(std::move(callback));
}

//-----------------------------------------------------------------------------
// ServerStream Implementation
//-----------------------------------------------------------------------------
ServerStream::ServerStream(void* stream)
    : stream_(stream) {
}

ServerStream::~ServerStream() = default;

void ServerStream::send(const std::vector<uint8_t>& data) {
  // Cast to ServerStream base class which both unidirectional and bidirectional inherit from
  static_cast<webtransport::ServerStream*>(stream_)->Send(data);
}

void ServerStream::setInterval(uint64_t interval_ms, std::function<void()> callback) {
  static_cast<webtransport::ServerStream*>(stream_)->setInterval(interval_ms, std::move(callback));
}

void ServerStream::onStreamRead(std::function<void(std::vector<uint8_t>)> callback) {
  static_cast<webtransport::ServerStream*>(stream_)->onStreamRead(std::move(callback));
}

} // namespace web_transport