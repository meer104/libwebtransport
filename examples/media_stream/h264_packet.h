#ifndef H264_PACKET_H_
#define H264_PACKET_H_

#include <cstdint>
#include <vector>

// Error codes
enum H264PacketError {
  H264_PACKET_OK = 0,
  H264_PACKET_ERROR_NIL_PACKET,
  H264_PACKET_ERROR_SHORT_PACKET,
  H264_PACKET_ERROR_UNHANDLED_NALU_TYPE,
};

// Constants for H264 NALU types and bitmasks
const uint8_t NALU_TYPE_BITMASK = 0x1F;
const uint8_t NALU_REF_IDC_BITMASK = 0x60;
const uint8_t FU_START_BITMASK = 0x80;
const uint8_t FU_END_BITMASK = 0x40;

const uint8_t STAPA_NALU_TYPE = 24;
const uint8_t FUA_NALU_TYPE = 28;
const uint8_t FUB_NALU_TYPE = 29;
const uint8_t SPS_NALU_TYPE = 7;
const uint8_t PPS_NALU_TYPE = 8;
const uint8_t AUD_NALU_TYPE = 9;
const uint8_t FILLER_NALU_TYPE = 12;

const uint8_t FUA_HEADER_SIZE = 2;
const uint8_t STAPA_HEADER_SIZE = 1;
const uint8_t STAPA_NALU_LENGTH_SIZE = 2;

// H264Packet represents the H264 header that is stored in the payload of an RTP Packet
class H264Packet {
 public:
  H264Packet();
  ~H264Packet();
  
  // Set if using AVC format rather than Annex B format
  void SetAVC(bool is_avc);
  bool IsAVC() const;
  
  // Unmarshal parses the passed byte slice and stores the result in the H264Packet
  H264PacketError Unmarshal(const uint8_t* payload, size_t payload_size, 
                          std::vector<uint8_t>* output_payload);
  
  // IsPartitionHead checks if this is a head of the H264 partition
  static bool IsPartitionHead(const uint8_t* payload, size_t payload_size);
  
  // Checks if the packet passed in has the marker bit set
  // indicating the end of a packet sequence
  static bool IsDetectedFinalPacketInSequence(bool rtp_packet_marker_bit);
  
 private:
  bool is_avc_;
  std::vector<uint8_t> fua_buffer_;
  
  // Helper methods
  H264PacketError ParseBody(const uint8_t* payload, size_t payload_size, 
                          std::vector<uint8_t>* output_payload);
  void DoPackaging(std::vector<uint8_t>* buffer, const uint8_t* nalu, size_t nalu_size);
};

#endif  // H264_PACKET_H_