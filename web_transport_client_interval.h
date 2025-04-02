#ifndef WEBTRANSPORT_INTERVAL_ALARM_H_
#define WEBTRANSPORT_INTERVAL_ALARM_H_

#include <functional>
#include "quiche/quic/core/quic_alarm.h"
#include "quiche/quic/core/quic_clock.h"

namespace webtransport
{

  // Interval Alarm Delegate
  class ClientIntervalAlarmDelegate : public quic::QuicAlarm::DelegateWithoutContext
  {
  public:
  ClientIntervalAlarmDelegate(const quic::QuicClock *clock, uint64_t interval_ms,
                          std::function<void()> callback);

    void OnAlarm() override;
    void SetAlarm(quic::QuicAlarm *alarm);

  private:
    const quic::QuicClock *clock_;
    quic::QuicAlarm *alarm_;
    uint64_t interval_ms_;
    std::function<void()> callback_;
  };

}

#endif