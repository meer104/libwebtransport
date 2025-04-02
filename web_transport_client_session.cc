#include "web_transport_client_session.h"
#include "web_transport_client_stream.h"
#include "web_transport_client_interval.h"

namespace webtransport
{

  // ClientSession implementation
  ClientSession::ClientSession(quic::WebTransportHttp3 *session,
                               quic::QuicAlarmFactory *alarm_factory,
                               const quic::QuicClock *clock)
      : session_(session), alarm_factory_(alarm_factory), clock_(clock)
  {
    session_->SetVisitor(std::make_unique<ClientSessionVisitor>(this));
  }

  ClientBidirectionalStream *ClientSession::createBidirectionalStream()
  {
    auto stream = session_->OpenOutgoingBidirectionalStream();
    return stream ? new ClientBidirectionalStream(stream, alarm_factory_, clock_) : nullptr;
  }

  void ClientSession::SendDatagram(const std::vector<uint8_t> &data)
  {
    std::string payload(data.begin(), data.end());
    session_->SendOrQueueDatagram(payload);
  }

  void ClientSession::setInterval(uint64_t interval_ms, std::function<void()> callback)
  {
    auto delegate = new ClientIntervalAlarmDelegate(clock_, interval_ms, std::move(callback));
    interval_alarm_.reset(alarm_factory_->CreateAlarm(delegate));
    delegate->SetAlarm(interval_alarm_.get());
    interval_alarm_->Set(clock_->Now() +
                         quic::QuicTime::Delta::FromMilliseconds(interval_ms));
  }

  void ClientSession::OnSessionClosed(webtransport::SessionErrorCode error_code,
                                      const std::string &error_message)
  {
    if (session_error_callback_)
    {
      session_error_callback_(error_message);
    }
    else
    {
      std::cout << "Session closed: " << error_message << std::endl;
    }
  }

  void ClientSession::OnDatagramReceived(absl::string_view datagram)
  {
    if (datagram_callback_)
    {
      std::vector<uint8_t> data(datagram.begin(), datagram.end());
      datagram_callback_(data);
    }
  }

  void ClientSession::OnIncomingBidirectionalStreamAvailable()
  {
    while (auto stream = session_->AcceptIncomingBidirectionalStream())
    {
      auto client_stream = new ClientBidirectionalStream(stream, alarm_factory_, clock_);
      if (bidi_stream_callback_)
      {
        bidi_stream_callback_(this, client_stream);
      }
    }
  }

  void ClientSession::OnIncomingUnidirectionalStreamAvailable()
  {
    // Implement similarly for unidirectional streams if needed.
  }

  void ClientSession::onDatagramRead(std::function<void(std::vector<uint8_t>)> callback)
  {
    datagram_callback_ = std::move(callback);
  }

  void ClientSession::onBidirectionalStream(
      std::function<void(ClientSession *, ClientBidirectionalStream *)> callback)
  {
    bidi_stream_callback_ = std::move(callback);
  }

  void ClientSession::setErrorCallback(std::function<void(std::string)> callback)
  {
    session_error_callback_ = std::move(callback);
  }

  // ClientSessionVisitor implementation
  ClientSessionVisitor::ClientSessionVisitor(ClientSession *session)
      : session_(session) {}

  void ClientSessionVisitor::OnSessionReady() {}

  void ClientSessionVisitor::OnSessionClosed(webtransport::SessionErrorCode error_code,
                                             const std::string &error_message)
  {
    if (session_)
      session_->OnSessionClosed(error_code, error_message);
  }

  void ClientSessionVisitor::OnDatagramReceived(absl::string_view datagram)
  {
    if (session_)
      session_->OnDatagramReceived(datagram);
  }

  void ClientSessionVisitor::OnIncomingBidirectionalStreamAvailable()
  {
    if (session_)
      session_->OnIncomingBidirectionalStreamAvailable();
  }

  void ClientSessionVisitor::OnIncomingUnidirectionalStreamAvailable()
  {
    if (session_)
      session_->OnIncomingUnidirectionalStreamAvailable();
  }

  void ClientSessionVisitor::OnCanCreateNewOutgoingBidirectionalStream() {}

  void ClientSessionVisitor::OnCanCreateNewOutgoingUnidirectionalStream() {}

} // namespace webtransport