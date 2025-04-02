#include "web_transport_client.h"
#include "web_transport_client_session.h"
#include "web_transport_client_stream.h"
#include "web_transport_client_interval.h"

#ifdef _WIN32
// Include Windows sockets header and link against ws2_32.lib
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace webtransport
{

  // Define these classes inside the implementation file to keep the header clean
  // Delegate for checking handshake progress.
  class Client::HandshakeAlarmDelegate : public quic::QuicAlarm::DelegateWithoutContext
  {
  public:
    HandshakeAlarmDelegate(Client *client, const quic::QuicClock *clock)
        : client_(client), clock_(clock) {}

    void OnAlarm() override
    {
      auto *session = client_->client_->client_session();
      if (session)
      {
        client_->CreateWebTransportSession();
      }
      else
      {
        alarm_->Set(clock_->Now() + quic::QuicTime::Delta::FromMilliseconds(10));
      }
    }

    void SetAlarm(quic::QuicAlarm *alarm) { alarm_ = alarm; }

  private:
    Client *client_;
    const quic::QuicClock *clock_;
    quic::QuicAlarm *alarm_;
  };

  // Delegate for checking session readiness.
  class Client::SessionReadyAlarmDelegate : public quic::QuicAlarm::DelegateWithoutContext
  {
  public:
    SessionReadyAlarmDelegate(Client *client, quic::QuicSpdyClientStream *stream,
                              const quic::QuicClock *clock)
        : client_(client), stream_(stream), clock_(clock) {}

    void OnAlarm() override
    {
      auto *session = client_->client_->client_session();

      // First check if the client session is still valid
      if (!session)
      {
        // Client session is gone, likely due to connection error
        if (client_->session_error_callback_)
        {
          client_->session_error_callback_("Connection lost or failed");
        }
        client_->connected_ = false; // Stop the event loop
        return;
      }

      auto *wt_session = session->GetWebTransportSession(stream_->id());

      // Check if the stream was reset or connection error
      if (stream_->stream_error() || stream_->connection_error())
      {
        std::string error_msg = "Stream reset or connection error";
        if (client_->session_error_callback_)
        {
          client_->session_error_callback_(error_msg);
        }
        else
        {
          std::cout << "Session error: " << error_msg << std::endl;
        }
        client_->connected_ = false; // Stop the event loop
        return;
      }

      // If the session exists and is ready, proceed normally
      if (wt_session && wt_session->ready())
      {
        auto client_session =
            new ClientSession(wt_session, client_->alarm_factory_.get(), client_->clock_);
        // Propagate any error callback.
        if (client_->session_error_callback_)
        {
          client_session->setErrorCallback(client_->session_error_callback_);
        }
        if (client_->session_callback_)
        {
          client_->session_callback_(client_session);
        }
        client_session->onBidirectionalStream(client_->bidi_stream_callback_);
      }
      // Check if the stream indicates rejection
      else if (stream_->headers_decompressed())
      {
        // Access headers differently (headers_decompressed instead of response_headers_complete)
        const auto &headers = stream_->response_headers();

        // Use string for status rather than string_view conversion
        std::string status;
        auto status_it = headers.find(":status");
        if (status_it != headers.end())
        {
          status = std::string(status_it->second);
        }
        else
        {
          status = "unknown";
        }

        std::string error_msg = "Server rejected WebTransport session with status: " + status;

        if (client_->session_error_callback_)
        {
          client_->session_error_callback_(error_msg);
        }
        else
        {
          std::cout << "Session error: " << error_msg << std::endl;
        }
        client_->connected_ = false; // Stop the event loop
      }
      // Otherwise, keep checking
      else
      {
        alarm_->Set(clock_->Now() + quic::QuicTime::Delta::FromMilliseconds(10));
      }
    }

    void SetAlarm(quic::QuicAlarm *alarm) { alarm_ = alarm; }

  private:
    Client *client_;
    quic::QuicSpdyClientStream *stream_;
    const quic::QuicClock *clock_;
    quic::QuicAlarm *alarm_;
  };

  Client::Client(const std::string &url)
  {
#ifdef _WIN32
    // Initialize Windows Sockets API
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
      throw std::runtime_error("WSAStartup failed with error: " + std::to_string(result));
    }
#endif

    event_loop_ = quic::GetDefaultEventLoop()->Create(quic::QuicDefaultClock::Get());
    clock_ = event_loop_->GetClock();
    alarm_factory_ = event_loop_->CreateAlarmFactory();
    ParseUrl(url);
    SetDefaultHeaders();
  }

  Client::~Client()
  {
#ifdef _WIN32
    WSACleanup();
#endif

    connected_ = false;
    event_loop_->RunEventLoopOnce(quic::QuicTime::Delta::Zero());
  }

  void Client::setHeader(const std::string &key, const std::string &value)
  {
    headers_[key] = value;
  }

  void Client::setHeaders(quiche::HttpHeaderBlock headers)
  {
    headers_ = std::move(headers);
  }

  void Client::setPublicKeyFile(std::string file_path)
  {
    public_key_file = file_path;
  }

  void Client::setCACertBundleFile(std::string file_path)
  {
    ca_cert_bundle_path = file_path;
  }

  void Client::setCACertDir(std::string dir_path)
  {
    ca_cert_dir = dir_path;
  }

  void Client::connect()
  {
    SetupNetworkComponents();
  }

  const quiche::HttpHeaderBlock &Client::getHeaders() const
  {
    return headers_;
  }

  void Client::onSessionOpen(std::function<void(ClientSession *)> callback)
  {
    session_callback_ = std::move(callback);
  }

  void Client::onBidirectionalStream(
      std::function<void(ClientSession *, ClientBidirectionalStream *)> callback)
  {
    bidi_stream_callback_ = std::move(callback);
  }

  void Client::onSessionError(std::function<void(std::string)> callback)
  {
    session_error_callback_ = std::move(callback);
  }

  void Client::runEventLoop() {
    while (connected_) {
      event_loop_->RunEventLoopOnce(quic::QuicTime::Delta::FromMilliseconds(50));
      //event_loop_->RunEventLoopOnce(quic::QuicTime::Delta::Infinite()); // or a reasonable timeout
    }
  }

  void Client::disconnect()
  {
    connected_ = false;
    // Clear any pending alarms
    if (handshake_alarm_)
    {
      handshake_alarm_->Cancel();
    }
    if (session_ready_alarm_)
    {
      session_ready_alarm_->Cancel();
    }

    // Give event loop a chance to process any final events
    event_loop_->RunEventLoopOnce(quic::QuicTime::Delta::Zero());

    // Clear the client which will close the connection
    client_.reset();
  }

  void Client::ParseUrl(const std::string &url_str)
  {
    url_ = quic::QuicUrl(url_str, "https");
    if (!url_.IsValid())
    {
      throw std::invalid_argument("Invalid WebTransport URL: " + url_str);
    }
  }

  void Client::SetDefaultHeaders()
  {
    headers_[":method"] = "CONNECT";
    headers_[":protocol"] = "webtransport";
    headers_[":scheme"] = url_.scheme();
    headers_[":path"] = url_.path();
    headers_[":authority"] = url_.HostPort();
    headers_["origin"] = absl::StrCat(url_.scheme(), "://", url_.host());
  }

  void Client::SetupNetworkComponents()
  {
    server_address_ = quic::tools::LookupAddress(
        AF_UNSPEC, url_.host(), std::to_string(url_.port()));

    quic::QuicConfig config;
    //config.SetMaxUnidirectionalStreamsToSend(1000);
    //config.set_max_time_before_crypto_handshake(
        //quic::QuicTime::Delta::FromMilliseconds(10000));


    config.SetIdleNetworkTimeout(quic::QuicTime::Delta::FromSeconds(quic::kMaximumIdleTimeoutSecs));
    config.SetMaxBidirectionalStreamsToSend(quic::kDefaultMaxStreamsPerConnection);
    config.SetMaxUnidirectionalStreamsToSend(quic::kDefaultMaxStreamsPerConnection);
    config.set_max_time_before_crypto_handshake(quic::QuicTime::Delta::FromSeconds(quic::kMaxTimeForCryptoHandshakeSecs));
    config.set_max_idle_time_before_crypto_handshake(quic::QuicTime::Delta::FromSeconds(quic::kInitialIdleTimeoutSecs));
    config.set_max_undecryptable_packets(quic::kDefaultMaxUndecryptablePackets);
    config.SetInitialStreamFlowControlWindowToSend(quic::kMinimumFlowControlSendWindow);
    config.SetInitialSessionFlowControlWindowToSend(quic::kMinimumFlowControlSendWindow);
    config.SetMaxAckDelayToSendMs(quic::GetDefaultDelayedAckTimeMs());
    config.SetAckDelayExponentToSend(quic::kDefaultAckDelayExponent);
    config.SetMaxPacketSizeToSend(quic::kMaxIncomingPacketSize);
    config.SetMaxDatagramFrameSizeToSend(quic::kMaxAcceptedDatagramFrameSize);
    config.SetReliableStreamReset(false);


    std::unique_ptr<quic::ProofVerifier> verifier;
    verifier = std::make_unique<webtransport::BoringSSLProofVerifier>(
        public_key_file, ca_cert_bundle_path, ca_cert_dir);

    client_ = std::make_unique<quic::QuicDefaultClient>(
        server_address_, quic::QuicServerId(url_.host(), url_.port()),
        quic::CurrentSupportedVersions(), config, event_loop_.get(),
        std::move(verifier),
        std::make_unique<quic::QuicClientSessionCache>());

    client_->set_enable_web_transport(true);
    client_->set_use_datagram_contexts(true);

    if (!client_->Initialize() || !client_->Connect())
    {
      throw std::runtime_error("Connection initialization failed");
    }

    StartHandshakeCheck();
  }

  void Client::StartHandshakeCheck()
  {
    auto delegate = new HandshakeAlarmDelegate(this, clock_);
    handshake_alarm_.reset(alarm_factory_->CreateAlarm(delegate));
    delegate->SetAlarm(handshake_alarm_.get());
    handshake_alarm_->Set(clock_->Now());
  }

  void Client::CreateWebTransportSession()
  {
    auto *session = client_->client_session();
    stream_ = session->CreateOutgoingBidirectionalStream();

    quiche::HttpHeaderBlock send_headers = headers_.Clone();
    stream_->SendRequest(std::move(send_headers), "", false);

    auto delegate = new SessionReadyAlarmDelegate(this, stream_, clock_);
    session_ready_alarm_.reset(alarm_factory_->CreateAlarm(delegate));
    delegate->SetAlarm(session_ready_alarm_.get());
    session_ready_alarm_->Set(clock_->Now());
  }

}