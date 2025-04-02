#include "web_transport_server_interval.h"

namespace webtransport
{

    IntervalAlarmDelegate::IntervalAlarmDelegate(const quic::QuicClock *clock, uint64_t interval_ms, std::function<void()> cb)
        : clock_(clock), interval_ms_(interval_ms), cb_(std::move(cb)), alarm_(nullptr) {}

    void IntervalAlarmDelegate::OnAlarm()
    {
        if (cb_)
        {
            cb_();
        }
        alarm_->Set(clock_->Now() + quic::QuicTime::Delta::FromMilliseconds(interval_ms_));
    }

    void IntervalAlarmDelegate::SetAlarm(quic::QuicAlarm *alarm)
    {
        alarm_ = alarm;
    }

}