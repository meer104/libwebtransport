#include "h264_rtp_depacketizer.h"

#include <cstring>
#include <iostream>

H264Depacketizer::H264Depacketizer() {}

H264Depacketizer::~H264Depacketizer() {}

void H264Depacketizer::SetAVC(bool is_avc) {
  packet_.SetAVC(is_avc);
}

bool H264Depacketizer::IsAVC() const {
  return packet_.IsAVC();
}

bool H264Depacketizer::Unmarshal(const uint8_t* packet, size_t packet_size, 
                               std::vector<uint8_t>* payload) {
  if (!packet || packet_size == 0 || !payload) {
    return false;
  }
  
  H264PacketError err = packet_.Unmarshal(packet, packet_size, payload);
  if (err != H264_PACKET_OK) {
    return false;
  }
  
  return true;
}

bool H264Depacketizer::IsPartitionHead(const uint8_t* payload, size_t payload_size) {
  return H264Packet::IsPartitionHead(payload, payload_size);
}

bool H264Depacketizer::IsPartitionTail(bool marker, const uint8_t* payload, size_t payload_size) {
  // For H264, the marker bit indicates the last packet of a frame
  return marker;
}

bool H264Depacketizer::IsKeyFrame(const uint8_t* payload, size_t payload_size) {
  if (payload_size < 1) {
    return false;
  }
  
  // Check if NALU type is 5 (IDR picture) or if it's a STAP-A (24) that may contain an IDR
  uint8_t nalu_type = payload[0] & NALU_TYPE_BITMASK;
  
  if (nalu_type == 5) {
    return true;
  }
  
  // Check for IDR frame inside STAP-A
  if (nalu_type == STAPA_NALU_TYPE && payload_size >= 3) {
    size_t current_offset = STAPA_HEADER_SIZE;
    
    while (current_offset + STAPA_NALU_LENGTH_SIZE < payload_size) {
      // Get NALU size
      uint16_t nalu_size = (static_cast<uint16_t>(payload[current_offset]) << 8) |
                            static_cast<uint16_t>(payload[current_offset + 1]);
      current_offset += STAPA_NALU_LENGTH_SIZE;
      
      // Check for IDR NALU type
      if (current_offset < payload_size && (payload[current_offset] & NALU_TYPE_BITMASK) == 5) {
        return true;
      }
      
      // Move to next NALU
      current_offset += nalu_size;
    }
  }
  
  // For FU-A, check if it's the first fragment of an IDR frame
  if (nalu_type == FUA_NALU_TYPE && payload_size >= 2) {
    bool is_start = (payload[1] & FU_START_BITMASK) != 0;
    uint8_t original_nalu_type = payload[1] & NALU_TYPE_BITMASK;
    
    if (is_start && original_nalu_type == 5) {
      return true;
    }
  }
  
  return false;
}