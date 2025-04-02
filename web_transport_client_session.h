#ifndef WEBTRANSPORT_CLIENT_SESSION_H_
#define WEBTRANSPORT_CLIENT_SESSION_H_

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "absl/strings/string_view.h"
#include "quiche/quic/core/http/web_transport_http3.h"
#include "quiche/quic/core/quic_alarm_factory.h"
#include "quiche/quic/core/quic_default_clock.h"


namespace webtransport
{

  // Forward declarations
  class ClientBidirectionalStream;
  class ClientSessionVisitor;

  // ClientSession implementation with error callback support
  class ClientSession
  {
  public:
    explicit ClientSession(quic::WebTransportHttp3 *session,
                           quic::QuicAlarmFactory *alarm_factory,
                           const quic::QuicClock *clock);

    ClientBidirectionalStream *createBidirectionalStream();
    void SendDatagram(const std::vector<uint8_t> &data);
    void setInterval(uint64_t interval_ms, std::function<void()> callback);

    // When the session is closed (e.g. rejected by the server),
    // call the error callback if one is set.
    void OnSessionClosed(webtransport::SessionErrorCode error_code,
                         const std::string &error_message);
    void OnDatagramReceived(absl::string_view datagram);
    void OnIncomingBidirectionalStreamAvailable();
    void OnIncomingUnidirectionalStreamAvailable();
    void onDatagramRead(std::function<void(std::vector<uint8_t>)> callback);
    void onBidirectionalStream(
        std::function<void(ClientSession *, ClientBidirectionalStream *)> callback);

    // New function: register error callback for session errors.
    void setErrorCallback(std::function<void(std::string)> callback);

  private:
    quic::WebTransportHttp3 *session_;
    quic::QuicAlarmFactory *alarm_factory_;
    const quic::QuicClock *clock_;
    std::unique_ptr<quic::QuicAlarm> interval_alarm_;
    std::function<void(std::vector<uint8_t>)> datagram_callback_;
    std::function<void(ClientSession *, ClientBidirectionalStream *)> bidi_stream_callback_;
    std::function<void(std::string)> session_error_callback_;
  };

  // ClientSession Visitor
  class ClientSessionVisitor : public quic::WebTransportVisitor
  {
  public:
    explicit ClientSessionVisitor(ClientSession *session);

    void OnSessionReady() override;
    void OnSessionClosed(webtransport::SessionErrorCode error_code,
                         const std::string &error_message) override;
    void OnDatagramReceived(absl::string_view datagram) override;
    void OnIncomingBidirectionalStreamAvailable() override;
    void OnIncomingUnidirectionalStreamAvailable() override;
    void OnCanCreateNewOutgoingBidirectionalStream() override;
    void OnCanCreateNewOutgoingUnidirectionalStream() override;

  private:
    ClientSession *session_;
  };

} // namespace webtransport

#endif // WEBTRANSPORT_CLIENT_SESSION_H_