#ifndef RTP_PACKET_H_
#define RTP_PACKET_H_

#include <cstdint>
#include <vector>

// RTPPacket represents an RTP packet as defined in RFC 3550
class RTPPacket {
 public:
  RTPPacket();
  ~RTPPacket();

  // Unmarshal parses the passed byte slice and stores the result in the RTPPacket
  bool Unmarshal(const uint8_t* buffer, size_t buffer_size);

  // GetPayload returns the packet payload
  const uint8_t* GetPayload() const { return payload_; }
  
  // GetPayloadSize returns the size of the payload
  size_t GetPayloadSize() const { return payload_size_; }

  // GetSequenceNumber returns the sequence number
  uint16_t GetSequenceNumber() const { return sequence_number_; }

  // GetTimestamp returns the timestamp
  uint32_t GetTimestamp() const { return timestamp_; }

  // GetSSRC returns the SSRC
  uint32_t GetSSRC() const { return ssrc_; }

  // GetPayloadType returns the payload type
  uint8_t GetPayloadType() const { return payload_type_; }

  // GetMarker returns the marker bit
  bool GetMarker() const { return marker_; }

  // GetVersion returns the version
  uint8_t GetVersion() const { return version_; }

  // GetPadding returns the padding bit
  bool GetPadding() const { return padding_; }

  // GetExtension returns the extension bit
  bool GetExtension() const { return extension_; }

  // GetCSRCCount returns the CSRC count
  uint8_t GetCSRCCount() const { return csrc_count_; }

  // GetExtensionHeaderID returns the extension header ID
  uint16_t GetExtensionHeaderID() const { return extension_header_id_; }

  // GetExtensionLength returns the extension length
  uint16_t GetExtensionLength() const { return extension_length_; }

  // GetExtensionHeaderValue returns the extension header value
  const uint8_t* GetExtensionHeaderValue() const { return extension_value_; }

 private:
  // Header fields
  uint8_t version_;          // 2 bits
  bool padding_;             // 1 bit
  bool extension_;           // 1 bit
  uint8_t csrc_count_;       // 4 bits
  bool marker_;              // 1 bit
  uint8_t payload_type_;     // 7 bits
  uint16_t sequence_number_; // 16 bits
  uint32_t timestamp_;       // 32 bits
  uint32_t ssrc_;            // 32 bits
  
  // CSRC list (Contributing sources)
  std::vector<uint32_t> csrcs_;
  
  // Extension header
  uint16_t extension_header_id_;
  uint16_t extension_length_;
  uint8_t* extension_value_;
  
  // Padding
  uint8_t padding_size_;
  
  // Payload
  uint8_t* payload_;
  size_t payload_size_;
};

#endif  // RTP_PACKET_H_