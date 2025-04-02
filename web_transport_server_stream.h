#pragma once

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace webtransport
{

    class ServerStream
    {
    public:
        virtual ~ServerStream() = default;

        virtual void Send(const std::vector<uint8_t> &data) = 0;

        virtual void setInterval(uint64_t interval_ms, std::function<void()> cb) = 0;

        using DataCallback = std::function<void(std::vector<uint8_t>)>;
        void onStreamRead(DataCallback cb) { data_cb_ = std::move(cb); }

    protected:
        DataCallback data_cb_;
    };

    class ServerUnidirectionalStream : public virtual ServerStream
    {
    public:
        // Implementation defined in the cpp file
    };

    class ServerBidirectionalStream : public virtual ServerStream
    {
    public:
        // Implementation defined in the cpp file
    };

} // namespace webtransport