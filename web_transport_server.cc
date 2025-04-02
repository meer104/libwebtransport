#include "web_transport_server.h"

#ifdef _WIN32
// Include Windows sockets header and link against ws2_32.lib
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <deque>

#include "absl/status/status.h"
#include "quiche/quic/core/web_transport_interface.h"
#include "quiche/quic/platform/api/quic_logging.h"
#include "quiche/common/platform/api/quiche_logging.h"
#include "quiche/common/quiche_circular_deque.h"
#include "quiche/common/quiche_stream.h"
#include "quiche/common/simple_buffer_allocator.h"
#include "quiche/web_transport/complete_buffer_visitor.h"
#include "quiche/web_transport/web_transport.h"

namespace webtransport
{

    // StreamWrapper implementation
    class Server::StreamWrapper : public quic::WebTransportStreamVisitor,
                                  public ServerUnidirectionalStream,
                                  public ServerBidirectionalStream
    {
    public:
        StreamWrapper(quic::WebTransportStream *stream, Server *server)
            : stream_(stream), server_(server) {}

        ~StreamWrapper() override {}

        void Send(const std::vector<uint8_t>& data) override {
            stream_->Write(absl::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
        }

        void setInterval(uint64_t interval_ms, std::function<void()> cb) override
        {
            auto *clock = server_->server_->event_loop()->GetClock();
            auto alarm_factory = server_->server_->event_loop()->CreateAlarmFactory();
            auto delegate = new IntervalAlarmDelegate(clock, interval_ms, cb);
            interval_alarm_.reset(alarm_factory->CreateAlarm(delegate));
            delegate->SetAlarm(interval_alarm_.get());
            interval_alarm_->Set(clock->Now() + quic::QuicTime::Delta::FromMilliseconds(interval_ms));
        }

        // WebTransportStreamVisitor overrides
        void OnCanRead() override
        {
            if (data_cb_)
            {
                std::string buffer;
                auto result = stream_->Read(&buffer);
                std::vector<uint8_t> data(buffer.begin(), buffer.end());
                data_cb_(data);
            }
        }

        void OnCanWrite() override {}
        void OnResetStreamReceived(quic::WebTransportStreamError) override {}
        void OnStopSendingReceived(quic::WebTransportStreamError) override {}
        void OnWriteSideInDataRecvdState() override {}

    private:
        quic::WebTransportStream* stream_;
        Server* server_;
        std::unique_ptr<quic::QuicAlarm> interval_alarm_;
        std::function<void()> fin_cb_;
        std::function<void(uint64_t error_code)> reset_cb_;
    };

    // SessionWrapper implementation
    class Server::SessionWrapper : public quic::WebTransportVisitor, public ServerSession
    {
    public:
        SessionWrapper(quic::WebTransportSession *session, Server *server, const std::string &path = "")
            : session_(session), server_(server), path_(path), session_closed_(false) {}

        ~SessionWrapper() override {}

        // ServerSession implementation
        void SendDatagram(const std::vector<uint8_t> &data) override
        {
            if (!session_closed_)
            {
                session_->SendOrQueueDatagram(absl::string_view(
                    reinterpret_cast<const char *>(data.data()), data.size()));
            }
        }

        void setInterval(uint64_t interval_ms, std::function<void()> cb) override
        {
            auto *clock = server_->server_->event_loop()->GetClock();
            auto alarm_factory = server_->server_->event_loop()->CreateAlarmFactory();
            auto delegate = new IntervalAlarmDelegate(clock, interval_ms, cb);
            interval_alarm_.reset(alarm_factory->CreateAlarm(delegate));
            delegate->SetAlarm(interval_alarm_.get());
            interval_alarm_->Set(clock->Now() + quic::QuicTime::Delta::FromMilliseconds(interval_ms));
        }

        // RejectSession implementation
        void RejectSession(uint32_t error_code, const std::string &reason) override
        {
            reject_error_code_ = error_code;
            reject_reason_ = reason;
            should_reject_ = true;

            // If OnSessionReady has already been called, we need to handle rejection differently
            if (onready_called_)
            {
                // Only close if not already closed
                if (!session_closed_)
                {
                    session_closed_ = true;
                    session_->CloseSession(error_code, reason);
                }
            }
            // If OnSessionReady hasn't been called yet, it will handle rejection when it's called
        }

        // WebTransportVisitor implementation
        void OnSessionReady() override
        {
            // Mark that this function has been called
            onready_called_ = true;

            // If we should reject this session (from a previous call to RejectSession)
            if (should_reject_)
            {
                if (!session_closed_)
                {
                    session_closed_ = true;
                    session_->CloseSession(reject_error_code_, reject_reason_);
                }
                return;
            }

            // Otherwise, call the session callback to determine if we should accept
            bool accept = true;
            if (server_->session_cb_)
            {
                accept = server_->session_cb_(this, path_);
            }

            // If the callback told us to reject, and RejectSession wasn't called from within the callback
            if (!accept && !should_reject_ && !session_closed_)
            {
                session_closed_ = true;
                session_->CloseSession(0, "Session rejected by application");
            }
        }

        void OnDatagramReceived(absl::string_view datagram) override
        {
            if (datagram_cb_ && !session_closed_)
            {
                std::vector<uint8_t> data(datagram.begin(), datagram.end());
                datagram_cb_(data);
            }
        }

        void OnIncomingUnidirectionalStreamAvailable() override
        {
            if (session_closed_)
                return;

            while (auto *stream = session_->AcceptIncomingUnidirectionalStream())
            {
                auto wrapper = std::make_unique<StreamWrapper>(stream, server_);
                ServerUnidirectionalStream *stream_ptr = wrapper.get();
                stream->SetVisitor(std::move(wrapper));

                if (server_->unidirectional_cb_)
                {
                    server_->unidirectional_cb_(this, stream_ptr, path_);
                }
            }
        }

        void OnIncomingBidirectionalStreamAvailable() override
        {
            if (session_closed_)
                return;

            while (auto *stream = session_->AcceptIncomingBidirectionalStream())
            {
                auto wrapper = std::make_unique<StreamWrapper>(stream, server_);
                ServerBidirectionalStream *stream_ptr = wrapper.get();
                stream->SetVisitor(std::move(wrapper));

                if (server_->bidirectional_cb_)
                {
                    server_->bidirectional_cb_(this, stream_ptr, path_);
                }
            }
        }

        void OnSessionClosed(quic::WebTransportSessionError error, const std::string &reason) override
        {
            session_closed_ = true;
        }

        void OnCanCreateNewOutgoingBidirectionalStream() override {}
        void OnCanCreateNewOutgoingUnidirectionalStream() override {}

    private:
        quic::WebTransportSession *session_;
        Server *server_;
        std::string path_;
        std::unique_ptr<quic::QuicAlarm> interval_alarm_;

        // Session state tracking
        bool onready_called_ = false;
        bool session_closed_ = false;
        bool should_reject_ = false;
        uint32_t reject_error_code_ = 0;
        std::string reject_reason_;
    };

    // Server implementation
    Server::Server(const std::string &host, uint16_t port)
        : host_(host), port_(port), server_initialized_(false) {}

    std::unique_ptr<quic::ProofSource> Server::CreateProofSource()
    {
        std::ifstream cert_stream(cert_file_, std::ios::binary);
        std::vector<std::string> certs = quic::CertificateView::LoadPemFromStream(&cert_stream);
        if (certs.empty())
        {
            QUICHE_LOG(ERROR) << "Failed to load certificates from " << cert_file_;
            exit(1);
        }

        std::ifstream key_stream(key_file_, std::ios::binary);
        auto private_key = quic::CertificatePrivateKey::LoadPemFromStream(&key_stream);
        if (!private_key)
        {
            QUICHE_LOG(ERROR) << "Failed to load private key from " << key_file_;
            exit(1);
        }

        auto chain = quiche::QuicheReferenceCountedPointer<quic::ProofSource::Chain>(
            new quic::ProofSource::Chain(certs));
        return quic::ProofSourceX509::Create(chain, std::move(*private_key));
    }

    void Server::InitializeServer()
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

        // Create an event loop
        quiche::QuicheSystemEventLoop event_loop("webtransport_server");

        // Create a backend that wraps each new session
        backend_ = std::make_unique<quic::WebTransportOnlyBackend>(
            [this](absl::string_view path, quic::WebTransportSession *session, quic::QuicServer *server)
            {
                // Store the path in the session wrapper instead of retrieving it later
                std::string path_str(path.begin(), path.end());
                auto wrapper = std::make_unique<SessionWrapper>(session, this, path_str);
                return wrapper;
            });

        auto proof_source = CreateProofSource();
        server_ = std::make_unique<quic::QuicServer>(std::move(proof_source), backend_.get());
        backend_->SetServer(server_.get());

        quic::QuicIpAddress ip;
        ip.FromString(host_);
        quic::QuicSocketAddress addr(ip, port_);

        if (!server_->CreateUDPSocketAndListen(addr))
        {
            QUICHE_LOG(ERROR) << "Failed to bind to " << addr.ToString();
            exit(1);
        }

        server_initialized_ = true;
    }

    void Server::Listen()
    {
        if (!server_initialized_)
        {
            QUICHE_LOG(ERROR) << "Server not initialized. Call InitializeServer() first.";
            return;
        }

        server_->HandleEventsForever();

#ifdef _WIN32
        WSACleanup();
#endif
    }

} // namespace webtransport
