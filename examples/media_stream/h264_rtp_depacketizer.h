#ifndef H264_RTP_DEPACKETIZER_H_
#define H264_RTP_DEPACKETIZER_H_

#include <cstdint>
#include <vector>

#include "h264_packet.h"

// H264Depacketizer depacketizes a H264 RTP payload
class H264Depacketizer {
 public:
  H264Depacketizer();
  ~H264Depacketizer();
  
  // Set if using AVC format rather than Annex B format
  void SetAVC(bool is_avc);
  bool IsAVC() const;
  
  // Unmarshal parses the RTP payload and returns H264 media
  bool Unmarshal(const uint8_t* packet, size_t packet_size, 
                std::vector<uint8_t>* payload);
  
  // IsPartitionHead checks if the packet is at the beginning of a partition
  bool IsPartitionHead(const uint8_t* payload, size_t payload_size);
  
  // IsPartitionTail checks if the packet is at the end of a partition
  bool IsPartitionTail(bool marker, const uint8_t* payload, size_t payload_size);
  
  // IsKeyFrame checks if the packet is a keyframe
  bool IsKeyFrame(const uint8_t* payload, size_t payload_size);

 private:
  H264Packet packet_;
};

#endif  // H264_RTP_DEPACKETIZER_H_