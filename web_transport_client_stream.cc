#include "web_transport_client_stream.h"
#include "web_transport_client_interval.h"

namespace webtransport
{

  ClientBidirectionalStream::ClientBidirectionalStream(quic::WebTransportStream *stream,
                                                       quic::QuicAlarmFactory *alarm_factory,
                                                       const quic::QuicClock *clock)
      : stream_(stream), alarm_factory_(alarm_factory), clock_(clock)
  {
    stream_->SetVisitor(std::make_unique<ClientStreamVisitor>(this));
  }

  bool ClientBidirectionalStream::Send(const std::vector<uint8_t> &data)
  {
    std::string payload(data.begin(), data.end());
    return stream_->Write(payload);
  }

  void ClientBidirectionalStream::onStreamRead(std::function<void(std::vector<uint8_t>)> callback)
  {
    read_callback_ = std::move(callback);
  }

  void ClientBidirectionalStream::setInterval(uint64_t interval_ms, std::function<void()> callback)
  {
    auto delegate = new ClientIntervalAlarmDelegate(clock_, interval_ms, std::move(callback));
    interval_alarm_.reset(alarm_factory_->CreateAlarm(delegate));
    delegate->SetAlarm(interval_alarm_.get());
    interval_alarm_->Set(clock_->Now() +
                         quic::QuicTime::Delta::FromMilliseconds(interval_ms));
  }

  quic::WebTransportStream *ClientBidirectionalStream::getStream()
  {
    return stream_;
  }

  std::function<void(std::vector<uint8_t>)> &ClientBidirectionalStream::getReadCallback()
  {
    return read_callback_;
  }

  // ClientStreamVisitor implementation
  ClientStreamVisitor::ClientStreamVisitor(ClientBidirectionalStream *stream)
      : stream_(stream) {}

      void ClientStreamVisitor::OnCanRead() {
        if (!stream_) {
            return;
        }
    
        quic::WebTransportStream* quicStream = stream_->getStream();
    
        // Handle the FIN-only case
        quic::WebTransportStream::PeekResult pr = quicStream->PeekNextReadableRegion();
        if (pr.fin_next && pr.peeked_data.empty()) {
          bool fin = quicStream->SkipBytes(0);
          if (fin && stream_->getReadCallback()) {
              // Send empty vector, to represent a fin with no data.
              std::cerr << "Stream FIN received (no data)." << std::endl;
              stream_->getReadCallback()({});
          }
          return;
        }
    
    
        std::vector<uint8_t> buffer; // Use a vector of uint8_t directly
        // Pre-allocate some reasonable size, we will resize later if we read less data.
        buffer.resize(quicStream->ReadableBytes() + 4096);  // Add extra space. Good initial size.
        size_t total_bytes_read = 0;
        bool fin_received = false;
        
        while (true) {
            pr = quicStream->PeekNextReadableRegion();
            if (pr.peeked_data.empty()) {
              break; // No more data (for now)
            }
            
            // Ensure that `len` does not exceed the buffer capacity
            size_t len = std::min(buffer.size() - total_bytes_read, pr.peeked_data.size());
    
            // If there isn't any space left in the buffer, break. This is very rare
            if (len == 0) {
              break;
            }
            
            // Copy the peeked data to our buffer
            std::memcpy(buffer.data() + total_bytes_read, pr.peeked_data.data(), len);
            
            // Consume the bytes from the stream
            bool fin = quicStream->SkipBytes(len);
            total_bytes_read += len;
    
            if (fin) {
              fin_received = true;
              break; // Stop if we see a FIN
            }
        }
    
    
        if (total_bytes_read > 0) {
          //Resize the buffer to actual read data
          buffer.resize(total_bytes_read);
          std::cerr << "ClientStreamVisitor::OnCanRead: total_bytes_read = " << total_bytes_read
                  << ", fin_received = " << fin_received << std::endl;
            if (stream_->getReadCallback()) {
                stream_->getReadCallback()(buffer);
            }
        } else if (fin_received) {
            //If fin and no bytes
            if (stream_->getReadCallback()) {
                std::cerr << "Stream FIN received (no data)." << std::endl;
                stream_->getReadCallback()({}); // fin, but no data
            }
        }
    }

  void ClientStreamVisitor::OnCanWrite() {}

  void ClientStreamVisitor::OnResetStreamReceived(quic::WebTransportStreamError error) {}

  void ClientStreamVisitor::OnStopSendingReceived(quic::WebTransportStreamError error) {}

  void ClientStreamVisitor::OnWriteSideInDataRecvdState() {}

} // namespace webtransport