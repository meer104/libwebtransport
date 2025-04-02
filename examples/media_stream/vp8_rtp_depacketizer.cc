#include "vp8_rtp_depacketizer.h"

#include <cstring>
#include <iostream>

VP8Depacketizer::VP8Depacketizer() {}

VP8Depacketizer::~VP8Depacketizer() {}

bool VP8Depacketizer::Unmarshal(const uint8_t* packet, size_t packet_size, 
                              std::vector<uint8_t>* payload) {
  if (!packet || packet_size == 0 || !payload) {
    return false;
  }
  
  VP8PacketError err = packet_.Unmarshal(packet, packet_size, payload);
  if (err != VP8_PACKET_OK) {
    return false;
  }
  
  return true;
}

bool VP8Depacketizer::IsPartitionHead(const uint8_t* payload, size_t payload_size) {
  return VP8Packet::IsPartitionHead(payload, payload_size);
}

bool VP8Depacketizer::IsPartitionTail(bool marker, const uint8_t* payload, size_t payload_size) {
  // For VP8, the marker bit indicates the last packet of a frame
  // So we just return the marker value directly
  return marker;
}

uint16_t VP8Depacketizer::GetPictureID() const {
  return packet_.PictureID();
}

bool VP8Depacketizer::IsKeyFrame() const {
  // In VP8, S bit must be 1 for the first packet of a frame
  // and partition ID (PID) should be 0 for a keyframe's first partition
  return packet_.S() == 1 && packet_.PID() == 0;
}