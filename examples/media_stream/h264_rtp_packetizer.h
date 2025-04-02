#ifndef H264_RTP_PACKETIZER_H_
#define H264_RTP_PACKETIZER_H_

#include <cstdint>
#include <vector>

// H264Payloader payloads H264 packets
class H264Payloader {
 public:
  H264Payloader();
  ~H264Payloader();
  
  // Set if using AVC format rather than Annex B format
  void SetAVC(bool is_avc);
  bool IsAVC() const;
  
  // Enable or disable STAP-A packets for aggregating NALUs
  void DisableSTAPA(bool disable);
  bool IsSTAPADisabled() const;
  
  // Sets the SPS (Sequence Parameter Set) NALU
  void SetSPS(const uint8_t* nalu, size_t nalu_size);
  
  // Sets the PPS (Picture Parameter Set) NALU
  void SetPPS(const uint8_t* nalu, size_t nalu_size);
  
  // Clear stored SPS/PPS NALUs
  void ClearSPSPPS();
  
  // Payload fragments a H264 packet across one or more byte arrays
  // mtu is the maximum size each fragment can have
  std::vector<std::vector<uint8_t>> Payload(uint16_t mtu, const uint8_t* payload, size_t payload_size);

 private:
  bool is_avc_;
  bool disable_stapa_;
  std::vector<uint8_t> sps_nalu_;
  std::vector<uint8_t> pps_nalu_;
  
  // Helper methods
  std::vector<std::vector<uint8_t>> CreateFUAPackets(uint16_t mtu, const uint8_t* nalu, size_t nalu_size);
  void EmitNALUs(const uint8_t* payload, size_t payload_size, 
                 std::vector<std::vector<uint8_t>>& payloads, uint16_t mtu);
};

#endif  // H264_RTP_PACKETIZER_H_