#ifndef WEBTRANSPORT_CLIENT_STREAM_H_
#define WEBTRANSPORT_CLIENT_STREAM_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "quiche/quic/core/quic_alarm.h"
#include "quiche/quic/core/quic_alarm_factory.h"
#include "quiche/quic/core/quic_time.h"
#include "quiche/quic/core/web_transport_interface.h"
#include "web_transport_client_interval.h"

namespace webtransport
{

  // Forward declarations
  class ClientStreamVisitor;

  // ClientBidirectionalStream declaration
  class ClientBidirectionalStream
  {
  public:
    explicit ClientBidirectionalStream(quic::WebTransportStream *stream,
                                       quic::QuicAlarmFactory *alarm_factory,
                                       const quic::QuicClock *clock);
    ~ClientBidirectionalStream() = default;

    bool Send(const std::vector<uint8_t> &data);
    void onStreamRead(std::function<void(std::vector<uint8_t>)> callback);
    void setInterval(uint64_t interval_ms, std::function<void()> callback);

    // Make these methods public so ClientStreamVisitor can access them
    quic::WebTransportStream *getStream();
    std::function<void(std::vector<uint8_t>)> &getReadCallback();

    struct PeekResult {
      bool has_data() const;  // True if there's data to read.
    
      // The next chunk of readable data (if any) as a string_view.
      // This is a VIEW into the stream's internal buffer; do NOT modify it!
      absl::string_view peeked_data;
    
      // True if a FIN is immediately after the peeked_data.
      bool fin_next;
    
      // True if this peek contains ALL data, including a FIN.
      // Important for knowing when the stream is completely read.
      bool all_data_received;
    };


  private:
    friend class ClientStreamVisitor;
    quic::WebTransportStream *stream_;
    quic::QuicAlarmFactory *alarm_factory_;
    const quic::QuicClock *clock_;
    std::unique_ptr<quic::QuicAlarm> interval_alarm_;
    std::function<void(std::vector<uint8_t>)> read_callback_;
  };

  // ClientStreamVisitor declaration
  class ClientStreamVisitor : public quic::WebTransportStreamVisitor
  {
  public:
    explicit ClientStreamVisitor(ClientBidirectionalStream *stream);

    void OnCanRead() override;
    void OnCanWrite() override;
    void OnResetStreamReceived(quic::WebTransportStreamError error) override;
    void OnStopSendingReceived(quic::WebTransportStreamError error) override;
    void OnWriteSideInDataRecvdState() override;

  private:
    ClientBidirectionalStream *stream_;
  };

} // namespace webtransport

#endif // WEBTRANSPORT_CLIENT_STREAM_H_