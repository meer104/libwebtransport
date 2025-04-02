#ifndef VP8_RTP_DEPACKETIZER_H_
#define VP8_RTP_DEPACKETIZER_H_

#include <cstdint>
#include <vector>

#include "vp8_packet.h"

// VP8Depacketizer depacketizes a VP8 RTP payload
class VP8Depacketizer {
 public:
  VP8Depacketizer();
  ~VP8Depacketizer();

  // Unmarshal parses the RTP payload and returns VP8 media
  bool Unmarshal(const uint8_t* packet, size_t packet_size, 
                std::vector<uint8_t>* payload);
  
  // IsPartitionHead checks if the packet is at the beginning of a partition
  bool IsPartitionHead(const uint8_t* payload, size_t payload_size);
  
  // IsPartitionTail checks if the packet is at the end of a partition
  bool IsPartitionTail(bool marker, const uint8_t* payload, size_t payload_size);
  
  // Returns current picture ID of the parsed packet
  uint16_t GetPictureID() const;
  
  // Returns whether the latest parsed packet is a keyframe
  bool IsKeyFrame() const;

 private:
  VP8Packet packet_;
};

#endif  // VP8_RTP_DEPACKETIZER_H_