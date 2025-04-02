#ifndef WEB_TRANSPORT_H_
#define WEB_TRANSPORT_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declarations to avoid including internal headers
namespace webtransport {
  class Client;
  class ClientSession;
  class ClientBidirectionalStream;
  class Server;
  class ServerSession;
  class ServerUnidirectionalStream;
  class ServerBidirectionalStream;
}

// Public API namespace to avoid conflicts with internal implementations
namespace web_transport {

//-----------------------------------------------------------------------------
// Client API
//-----------------------------------------------------------------------------
class Client {
public:
  // Constructor that takes a WebTransport URL
  Client(const std::string& url);
  ~Client();

  // Connection setup
  void setHeader(const std::string& key, const std::string& value);
  void setPublicKeyFile(const std::string& file_path);
  void setCACertBundleFile(const std::string& file_path);
  void setCACertDir(const std::string& dir_path);
  void connect();
  void disconnect();
  void runEventLoop();

  // Callback registration
  void onSessionOpen(std::function<void(void*)> callback);
  void onBidirectionalStream(std::function<void(void*, void*)> callback);
  void onSessionError(std::function<void(const std::string&)> callback);

private:
  std::unique_ptr<webtransport::Client> client_;

  // Wrapper callbacks to hide internal types from public API
  std::function<void(void*)> session_callback_;
  std::function<void(void*, void*)> bidi_stream_callback_;
  std::function<void(const std::string&)> session_error_callback_;
};

//-----------------------------------------------------------------------------
// ClientSession API
//-----------------------------------------------------------------------------
class ClientSession {
public:
  ClientSession(void* session);
  ~ClientSession();

  void* createBidirectionalStream();
  void sendDatagram(const std::vector<uint8_t>& data);
  void setInterval(uint64_t interval_ms, std::function<void()> callback);
  void onDatagramRead(std::function<void(std::vector<uint8_t>)> callback);
  void onBidirectionalStream(std::function<void(void*, void*)> callback);

private:
  webtransport::ClientSession* session_;
  std::function<void(void*, void*)> bidi_stream_callback_;
};

//-----------------------------------------------------------------------------
// ClientStream API
//-----------------------------------------------------------------------------
class ClientStream {
public:
  ClientStream(void* stream);
  ~ClientStream();

  bool send(const std::vector<uint8_t>& data);
  void onStreamRead(std::function<void(std::vector<uint8_t>)> callback);
  void setInterval(uint64_t interval_ms, std::function<void()> callback);

private:
  webtransport::ClientBidirectionalStream* stream_;
};

//-----------------------------------------------------------------------------
// Server API
//-----------------------------------------------------------------------------
class Server {
public:
  Server(const std::string& host, uint16_t port);
  ~Server();

  // Server configuration
  void setCertFile(const std::string& cert_file);
  void setKeyFile(const std::string& key_file);
  
  // Event handlers
  void onSession(std::function<bool(void*, const std::string&)> callback);
  void onUnidirectionalStream(std::function<void(void*, void*, const std::string&)> callback);
  void onBidirectionalStream(std::function<void(void*, void*, const std::string&)> callback);
  
  // Server lifecycle
  void initialize();
  void listen();

private:
  std::unique_ptr<webtransport::Server> server_;
  
  // Wrapper callbacks
  std::function<bool(void*, const std::string&)> session_callback_;
  std::function<void(void*, void*, const std::string&)> unidirectional_callback_;
  std::function<void(void*, void*, const std::string&)> bidirectional_callback_;
};

//-----------------------------------------------------------------------------
// ServerSession API
//-----------------------------------------------------------------------------
class ServerSession {
public:
  ServerSession(void* session);
  ~ServerSession();

  void sendDatagram(const std::vector<uint8_t>& data);
  void setInterval(uint64_t interval_ms, std::function<void()> callback);
  void rejectSession(uint32_t error_code = 0, const std::string& reason = "");
  void onDatagramRead(std::function<void(std::vector<uint8_t>)> callback);

private:
  webtransport::ServerSession* session_;
};

//-----------------------------------------------------------------------------
// ServerStream API
//-----------------------------------------------------------------------------
class ServerStream {
public:
  ServerStream(void* stream);
  ~ServerStream();

  void send(const std::vector<uint8_t>& data);
  void setInterval(uint64_t interval_ms, std::function<void()> callback);
  void onStreamRead(std::function<void(std::vector<uint8_t>)> callback);

private:
  void* stream_; // Can be either ServerUnidirectionalStream or ServerBidirectionalStream
};

} // namespace web_transport

#endif // WEB_TRANSPORT_H_