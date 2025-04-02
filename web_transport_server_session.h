#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "web_transport_server_backend.h"
#include "web_transport_server_core.h"


namespace webtransport
{

    class ServerSession
    {
    public:
        virtual ~ServerSession() = default;

        virtual void SendDatagram(const std::vector<uint8_t> &data) = 0;

        virtual void setInterval(uint64_t interval_ms, std::function<void()> cb) = 0;

        // Method to reject the session
        virtual void RejectSession(uint32_t error_code = 0, const std::string &reason = "") = 0;

        using DatagramCallback = std::function<void(std::vector<uint8_t>)>;
        void onDatagramRead(DatagramCallback cb) { datagram_cb_ = std::move(cb); }

    protected:
        DatagramCallback datagram_cb_;
    };

}