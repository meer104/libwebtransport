#include "h264_rtp_packetizer.h"

#include <algorithm>
#include <cstring>
#include "h264_packet.h"

// NALU start codes
const uint8_t ANNEXB_NALU_START_CODE[4] = {0x00, 0x00, 0x00, 0x01};
const uint8_t NALU_START_CODE[3] = {0x00, 0x00, 0x01};

H264Payloader::H264Payloader() 
    : is_avc_(false),
      disable_stapa_(false) {}

H264Payloader::~H264Payloader() {}

void H264Payloader::SetAVC(bool is_avc) {
  is_avc_ = is_avc;
}

bool H264Payloader::IsAVC() const {
  return is_avc_;
}

void H264Payloader::DisableSTAPA(bool disable) {
  disable_stapa_ = disable;
}

bool H264Payloader::IsSTAPADisabled() const {
  return disable_stapa_;
}

void H264Payloader::SetSPS(const uint8_t* nalu, size_t nalu_size) {
  sps_nalu_.clear();
  if (nalu && nalu_size > 0) {
    sps_nalu_.assign(nalu, nalu + nalu_size);
  }
}

void H264Payloader::SetPPS(const uint8_t* nalu, size_t nalu_size) {
  pps_nalu_.clear();
  if (nalu && nalu_size > 0) {
    pps_nalu_.assign(nalu, nalu + nalu_size);
  }
}

void H264Payloader::ClearSPSPPS() {
  sps_nalu_.clear();
  pps_nalu_.clear();
}

std::vector<std::vector<uint8_t>> H264Payloader::CreateFUAPackets(
    uint16_t mtu, const uint8_t* nalu, size_t nalu_size) {
  std::vector<std::vector<uint8_t>> packets;
  
  if (nalu_size <= 1) {
    return packets;  // Invalid NALU
  }
  
  // The FU-A header size
  const size_t fua_header_size = 2;
  
  // The first byte of the NALU (containing type)
  uint8_t nalu_header = nalu[0];
  
  // NALU type and NRI values
  uint8_t nalu_type = nalu_header & NALU_TYPE_BITMASK;
  uint8_t nalu_ref_idc = nalu_header & NALU_REF_IDC_BITMASK;
  
  // Maximum fragment size
  size_t max_fragment_size = mtu - fua_header_size;
  
  // Skip the first byte of the NALU (we'll reconstruct it in the FU-A packets)
  size_t data_remaining = nalu_size - 1;
  size_t data_index = 1;
  
  // Create FU-A packets until we've sent all the data
  bool first_fragment = true;
  
  while (data_remaining > 0) {
    // Size of current fragment
    size_t current_fragment_size = std::min(max_fragment_size, data_remaining);
    
    // Create packet for this fragment
    std::vector<uint8_t> packet(fua_header_size + current_fragment_size);
    
    // FU indicator - uses the original NALU NRI value but with FU-A type (28)
    packet[0] = nalu_ref_idc | FUA_NALU_TYPE;
    
    // FU header - contains original NALU type and start/end markers
    packet[1] = nalu_type;  // Original NALU type
    
    if (first_fragment) {
      packet[1] |= FU_START_BITMASK;  // Start bit
      first_fragment = false;
    }
    
    if (data_remaining == current_fragment_size) {
      packet[1] |= FU_END_BITMASK;  // End bit
    }
    
    // Copy the fragment data
    std::memcpy(packet.data() + fua_header_size, 
               nalu + data_index, 
               current_fragment_size);
    
    packets.push_back(std::move(packet));
    
    data_remaining -= current_fragment_size;
    data_index += current_fragment_size;
  }
  
  return packets;
}

void H264Payloader::EmitNALUs(const uint8_t* payload, size_t payload_size, 
                             std::vector<std::vector<uint8_t>>& payloads, uint16_t mtu) {
  // First, try to find a 3-byte or 4-byte start code
  size_t start = 0;
  
  while (start < payload_size) {
    // Find the next start code
    size_t next_start = payload_size;
    
    // Look for 3-byte start code
    for (size_t i = start + 3; i < payload_size; i++) {
      if (i + 2 < payload_size && 
          payload[i] == 0 && payload[i+1] == 0 && payload[i+2] == 1) {
        next_start = i;
        break;
      }
      
      // Also check for 4-byte start code
      if (i + 3 < payload_size &&
          payload[i-1] == 0 && payload[i] == 0 && payload[i+1] == 0 && payload[i+2] == 1) {
        next_start = i - 1;
        break;
      }
    }
    
    // Determine the offset from the start code to the actual NALU
    size_t offset = 3;  // Default for 3-byte start code
    
    // Check if we have a 3-byte or 4-byte start code
    if (start + 3 < payload_size && 
        payload[start] == 0 && payload[start+1] == 0 && 
        payload[start+2] == 0 && payload[start+3] == 1) {
      offset = 4;  // 4-byte start code
    } else if (start + 2 < payload_size &&
              payload[start] == 0 && payload[start+1] == 0 && payload[start+2] == 1) {
      offset = 3;  // 3-byte start code
    } else if (start == 0) {
      // No start code at the beginning, treat the entire buffer as one NALU
      offset = 0;
    }
    
    // Extract the NALU from start+offset to next_start
    const uint8_t* nalu = payload + start + offset;
    size_t nalu_size = next_start - (start + offset);
    
    if (nalu_size > 0) {
      // Check NALU type
      uint8_t nalu_type = nalu[0] & NALU_TYPE_BITMASK;
      
      // Skip AUD and filler NALUs
      if (nalu_type == AUD_NALU_TYPE || nalu_type == FILLER_NALU_TYPE) {
        // Skip this NALU
      } 
      // Store SPS/PPS if not disabled
      else if (!disable_stapa_ && nalu_type == SPS_NALU_TYPE) {
        SetSPS(nalu, nalu_size);
      }
      else if (!disable_stapa_ && nalu_type == PPS_NALU_TYPE) {
        SetPPS(nalu, nalu_size);
      } 
      // For other NALUs, check if we need to create a STAP-A packet with SPS/PPS
      else if (!disable_stapa_ && !sps_nalu_.empty() && !pps_nalu_.empty()) {
        // Calculate size of a STAP-A packet with SPS+PPS+current NALU
        size_t stapa_size = 1 +  // STAP-A header
                          2 + sps_nalu_.size() +  // Size + SPS
                          2 + pps_nalu_.size() +  // Size + PPS
                          2 + nalu_size;         // Size + current NALU
                          
        if (stapa_size <= mtu) {
          // We can fit SPS+PPS+NALU in one STAP-A packet
          std::vector<uint8_t> stapa_packet(stapa_size);
          
          // STAP-A header
          stapa_packet[0] = 0x78;  // STAP-A NALU type (24) + NRI bits
          
          // Add SPS size and data
          size_t offset = 1;
          stapa_packet[offset++] = (sps_nalu_.size() >> 8) & 0xFF;
          stapa_packet[offset++] = sps_nalu_.size() & 0xFF;
          std::memcpy(stapa_packet.data() + offset, sps_nalu_.data(), sps_nalu_.size());
          offset += sps_nalu_.size();
          
          // Add PPS size and data
          stapa_packet[offset++] = (pps_nalu_.size() >> 8) & 0xFF;
          stapa_packet[offset++] = pps_nalu_.size() & 0xFF;
          std::memcpy(stapa_packet.data() + offset, pps_nalu_.data(), pps_nalu_.size());
          offset += pps_nalu_.size();
          
          // Add current NALU size and data
          stapa_packet[offset++] = (nalu_size >> 8) & 0xFF;
          stapa_packet[offset++] = nalu_size & 0xFF;
          std::memcpy(stapa_packet.data() + offset, nalu, nalu_size);
          
          payloads.push_back(std::move(stapa_packet));
          
          // Clear SPS/PPS after use
          sps_nalu_.clear();
          pps_nalu_.clear();
          
          // Skip to the next NALU
          start = next_start;
          continue;
        }
      }
      
      // If the NALU fits in a single packet, send it as is
      if (nalu_size <= mtu) {
        std::vector<uint8_t> packet(nalu_size);
        std::memcpy(packet.data(), nalu, nalu_size);
        payloads.push_back(std::move(packet));
      } else {
        // NALU is too big, fragment it using FU-A
        auto fragments = CreateFUAPackets(mtu, nalu, nalu_size);
        payloads.insert(payloads.end(), 
                      std::make_move_iterator(fragments.begin()), 
                      std::make_move_iterator(fragments.end()));
      }
    }
    
    // Move to next NALU
    start = next_start;
  }
}

std::vector<std::vector<uint8_t>> H264Payloader::Payload(uint16_t mtu, 
                                                        const uint8_t* payload, 
                                                        size_t payload_size) {
  std::vector<std::vector<uint8_t>> payloads;
  
  if (!payload || payload_size == 0) {
    return payloads;
  }
  
  // Process the H264 stream and emit NALUs
  EmitNALUs(payload, payload_size, payloads, mtu);
  
  return payloads;
}