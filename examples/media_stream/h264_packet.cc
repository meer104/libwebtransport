#include "h264_packet.h"

#include <cstring>
#include <algorithm>

// NALU start codes
const uint8_t ANNEXB_NALU_START_CODE[4] = {0x00, 0x00, 0x00, 0x01};
const uint8_t NALU_START_CODE[3] = {0x00, 0x00, 0x01};

H264Packet::H264Packet() : is_avc_(false) {}

H264Packet::~H264Packet() {}

void H264Packet::SetAVC(bool is_avc) {
  is_avc_ = is_avc;
}

bool H264Packet::IsAVC() const {
  return is_avc_;
}

H264PacketError H264Packet::Unmarshal(const uint8_t* payload, size_t payload_size,
                                    std::vector<uint8_t>* output_payload) {
  if (!payload || payload_size == 0) {
    return H264_PACKET_ERROR_NIL_PACKET;
  }
  
  return ParseBody(payload, payload_size, output_payload);
}

bool H264Packet::IsPartitionHead(const uint8_t* payload, size_t payload_size) {
  if (payload_size < 2) {
    return false;
  }
  
  uint8_t nalu_type = payload[0] & NALU_TYPE_BITMASK;
  
  if (nalu_type == FUA_NALU_TYPE || nalu_type == FUB_NALU_TYPE) {
    return (payload[1] & FU_START_BITMASK) != 0;
  }
  
  return true;
}

bool H264Packet::IsDetectedFinalPacketInSequence(bool rtp_packet_marker_bit) {
  return rtp_packet_marker_bit;
}

void H264Packet::DoPackaging(std::vector<uint8_t>* buffer, const uint8_t* nalu, size_t nalu_size) {
  if (is_avc_) {
    // Add 4-byte length prefix for AVC format
    uint32_t length = static_cast<uint32_t>(nalu_size);
    buffer->push_back((length >> 24) & 0xFF);
    buffer->push_back((length >> 16) & 0xFF);
    buffer->push_back((length >> 8) & 0xFF);
    buffer->push_back(length & 0xFF);
    
    // Append NALU
    buffer->insert(buffer->end(), nalu, nalu + nalu_size);
  } else {
    // Use Annex B format with start code
    buffer->insert(buffer->end(), ANNEXB_NALU_START_CODE, ANNEXB_NALU_START_CODE + 4);
    buffer->insert(buffer->end(), nalu, nalu + nalu_size);
  }
}

H264PacketError H264Packet::ParseBody(const uint8_t* payload, size_t payload_size,
                                    std::vector<uint8_t>* output_payload) {
  if (payload_size < 1) {
    return H264_PACKET_ERROR_SHORT_PACKET;
  }
  
  output_payload->clear();
  
  // Get NALU type
  uint8_t nalu_type = payload[0] & NALU_TYPE_BITMASK;
  
  // Handle based on NALU type
  switch (nalu_type) {
    case 1:  // Coded slice of a non-IDR picture
    case 2:  // Coded slice data partition A
    case 3:  // Coded slice data partition B
    case 4:  // Coded slice data partition C
    case 5:  // Coded slice of an IDR picture
    case 6:  // Supplemental enhancement information
    case 7:  // Sequence parameter set
    case 8:  // Picture parameter set
    case 9:  // Access unit delimiter
    case 10: // End of sequence
    case 11: // End of stream
    case 12: // Filler data
    case 19: // Auxiliary coded picture
    case 20: // Extension
    case 21: // Depth map
    case 22: // Reserved
    case 23: // Reserved
      // Single NALU - just package it
      DoPackaging(output_payload, payload, payload_size);
      return H264_PACKET_OK;
      
    case STAPA_NALU_TYPE: {
      // STAP-A (Single-time aggregation packet)
      size_t current_offset = STAPA_HEADER_SIZE;
      
      while (current_offset < payload_size) {
        // Ensure we have enough bytes for the NALU length
        if (current_offset + STAPA_NALU_LENGTH_SIZE > payload_size) {
          break;
        }
        
        // Get NALU size (16-bit big endian)
        uint16_t nalu_size = (static_cast<uint16_t>(payload[current_offset]) << 8) |
                             static_cast<uint16_t>(payload[current_offset + 1]);
        current_offset += STAPA_NALU_LENGTH_SIZE;
        
        // Validate NALU size
        if (current_offset + nalu_size > payload_size) {
          return H264_PACKET_ERROR_SHORT_PACKET;
        }
        
        // Package this NALU
        DoPackaging(output_payload, payload + current_offset, nalu_size);
        
        // Move to next NALU
        current_offset += nalu_size;
      }
      
      return H264_PACKET_OK;
    }
      
    case FUA_NALU_TYPE: {
      // Fragmentation Units A (FU-A)
      if (payload_size < FUA_HEADER_SIZE) {
        return H264_PACKET_ERROR_SHORT_PACKET;
      }
      
      // Add to fragmentation buffer
      fua_buffer_.insert(fua_buffer_.end(), 
                       payload + FUA_HEADER_SIZE, 
                       payload + payload_size);
      
      // Check if this is the end of the fragmented packet
      if (payload[1] & FU_END_BITMASK) {
        // Reconstruct original NALU
        uint8_t nalu_ref_idc = payload[0] & NALU_REF_IDC_BITMASK;
        uint8_t fragmented_nalu_type = payload[1] & NALU_TYPE_BITMASK;
        
        // Create the original NALU header
        std::vector<uint8_t> complete_nalu;
        complete_nalu.push_back(nalu_ref_idc | fragmented_nalu_type);
        
        // Add the FU-A data
        complete_nalu.insert(complete_nalu.end(), 
                           fua_buffer_.begin(), 
                           fua_buffer_.end());
        
        // Package the complete NALU
        DoPackaging(output_payload, complete_nalu.data(), complete_nalu.size());
        
        // Clear buffer for next fragmented packet
        fua_buffer_.clear();
        return H264_PACKET_OK;
      }
      
      // Not the end fragment yet, return empty
      return H264_PACKET_OK;
    }
      
    default:
      return H264_PACKET_ERROR_UNHANDLED_NALU_TYPE;
  }
}