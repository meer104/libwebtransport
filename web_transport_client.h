#ifndef WEBTRANSPORT_CLIENT_H_
#define WEBTRANSPORT_CLIENT_H_

#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <stdexcept>
#include <vector>
#include <utility>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "quiche/quic/core/io/quic_default_event_loop.h"
#include "quiche/quic/core/quic_default_clock.h"
#include "quiche/quic/core/quic_alarm_factory.h"
#include "quiche/quic/tools/quic_default_client.h"
#include "quiche/quic/tools/quic_url.h"
#include "quiche/quic/tools/quic_name_lookup.h"
#include "quiche/quic/platform/api/quic_socket_address.h"
#include "quiche/common/http/http_header_block.h"
#include "quiche/quic/core/crypto/proof_verifier.h"
#include "quiche/quic/core/crypto/quic_client_session_cache.h"
#include "quiche/quic/core/quic_versions.h"
#include "quiche/quic/core/quic_server_id.h"
#include "quiche/web_transport/web_transport.h"
#include "web_transport_client_verify.h"

namespace webtransport
{

  // Forward declarations
  class ClientSession;
  class ClientBidirectionalStream;

  // Client class with session error callback support
  class Client
  {
  public:
    explicit Client(const std::string &url);
    ~Client();

    // Header management interface
    void setHeader(const std::string &key, const std::string &value);
    void setHeaders(quiche::HttpHeaderBlock headers);
    void setPublicKeyFile(std::string file_path);
    void setCACertBundleFile(std::string file_path);
    void setCACertDir(std::string dir_path);
    void connect();
    const quiche::HttpHeaderBlock &getHeaders() const;

    // Session lifecycle callbacks
    void onSessionOpen(std::function<void(ClientSession *)> callback);
    void onBidirectionalStream(
        std::function<void(ClientSession *, ClientBidirectionalStream *)> callback);
    void onSessionError(std::function<void(std::string)> callback);
    void runEventLoop();
    void disconnect();

  private:
    // Delegate for checking handshake progress.
    class HandshakeAlarmDelegate;

    // Delegate for checking session readiness.
    class SessionReadyAlarmDelegate;

    void ParseUrl(const std::string &url_str);
    void SetDefaultHeaders();
    void SetupNetworkComponents();
    void StartHandshakeCheck();
    void CreateWebTransportSession();

    std::string ca_cert_dir;
    std::string ca_cert_bundle_path;
    std::string public_key_file;
    std::unique_ptr<quic::QuicEventLoop> event_loop_;
    const quic::QuicClock *clock_;
    std::unique_ptr<quic::QuicAlarmFactory> alarm_factory_;
    quic::QuicUrl url_;
    quiche::HttpHeaderBlock headers_;
    std::unique_ptr<quic::QuicDefaultClient> client_;
    quic::QuicSocketAddress server_address_;
    quic::QuicSpdyClientStream *stream_ = nullptr;
    bool connected_ = true;
    std::function<void(ClientSession *)> session_callback_;
    std::function<void(ClientSession *, ClientBidirectionalStream *)> bidi_stream_callback_;
    std::function<void(std::string)> session_error_callback_;
    std::unique_ptr<quic::QuicAlarm> handshake_alarm_;
    std::unique_ptr<quic::QuicAlarm> session_ready_alarm_;

    friend class HandshakeAlarmDelegate;
    friend class SessionReadyAlarmDelegate;
  };

} // namespace webtransport

#endif // WEBTRANSPORT_CLIENT_H_