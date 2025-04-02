#pragma once

#include <cstdint>
#include <functional>
#include "quiche/quic/core/quic_alarm.h"
#include "web_transport_server_backend.h"
#include "web_transport_server_core.h"

namespace webtransport
{

    class IntervalAlarmDelegate : public quic::QuicAlarm::DelegateWithoutContext
    {
    public:
        IntervalAlarmDelegate(const quic::QuicClock *clock, uint64_t interval_ms, std::function<void()> cb);

        void OnAlarm() override;
        void SetAlarm(quic::QuicAlarm *alarm);

    private:
        const quic::QuicClock *clock_;
        quic::QuicAlarm *alarm_;
        uint64_t interval_ms_;
        std::function<void()> cb_;
    };

} // namespace webtransport