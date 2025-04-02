#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <fstream>
#include "quiche/common/platform/api/quiche_system_event_loop.h"
#include "web_transport_server_backend.h"
#include "web_transport_server_core.h"
#include "web_transport_server_interval.h"
#include "web_transport_server_session.h"
#include "web_transport_server_stream.h"
#include "quiche/quic/core/crypto/proof_source.h"
#include "quiche/quic/core/crypto/proof_source_x509.h"

// Forward declare webtransport classes
namespace webtransport
{
    class ServerSession;
    class ServerUnidirectionalStream;
    class ServerBidirectionalStream;
    class IntervalAlarmDelegate;
}

namespace webtransport
{

    class Server
    {
    public:
        // Modified callback type to return boolean (true = accept, false = reject)
        using SessionCallback = std::function<bool(ServerSession *, std::string)>;
        using UnidirectionalStreamCallback = std::function<void(ServerSession *, ServerUnidirectionalStream *, std::string)>;
        using BidirectionalStreamCallback = std::function<void(ServerSession *, ServerBidirectionalStream *, std::string)>;

        Server(const std::string &host, uint16_t port);
        ~Server() = default;

        // Certificate configuration
        void setCertFile(const std::string &cert_file) { cert_file_ = cert_file; }
        void setKeyFile(const std::string &key_file) { key_file_ = key_file; }

        // Event handlers
        void onSession(SessionCallback cb) { session_cb_ = std::move(cb); }
        void onUnidirectionalStream(UnidirectionalStreamCallback cb) { unidirectional_cb_ = std::move(cb); }
        void onBidirectionalStream(BidirectionalStreamCallback cb) { bidirectional_cb_ = std::move(cb); }

        // Server lifecycle methods
        void InitializeServer();
        void Listen();

    private:
        // Private implementation classes
        class SessionWrapper;
        class StreamWrapper;

        // Create proof source for SSL/TLS
        std::unique_ptr<quic::ProofSource> CreateProofSource();

        // Server configuration
        std::string host_;
        uint16_t port_;
        std::string cert_file_;
        std::string key_file_;

        // QUIC server components
        std::unique_ptr<quic::QuicServer> server_;
        std::unique_ptr<quic::WebTransportOnlyBackend> backend_;
        bool server_initialized_;

        // Callbacks
        SessionCallback session_cb_;
        UnidirectionalStreamCallback unidirectional_cb_;
        BidirectionalStreamCallback bidirectional_cb_;

        // Friend classes
        friend class SessionWrapper;
        friend class StreamWrapper;
    };

} // namespace webtransport