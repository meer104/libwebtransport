#include "web_transport_client_interval.h"

namespace webtransport
{

  ClientIntervalAlarmDelegate::ClientIntervalAlarmDelegate(const quic::QuicClock *clock, uint64_t interval_ms,
                                               std::function<void()> callback)
      : clock_(clock), interval_ms_(interval_ms), callback_(std::move(callback)) {}

  void ClientIntervalAlarmDelegate::OnAlarm()
  {
    if (callback_)
      callback_();
    alarm_->Set(clock_->Now() +
                quic::QuicTime::Delta::FromMilliseconds(interval_ms_));
  }

  void ClientIntervalAlarmDelegate::SetAlarm(quic::QuicAlarm *alarm)
  {
    alarm_ = alarm;
  }

}