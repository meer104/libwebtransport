#ifndef VP8_RTP_PACKETIZER_H_
#define VP8_RTP_PACKETIZER_H_

#include <cstdint>
#include <vector>

// VP8Payloader is responsible for creating VP8 RTP packets from a VP8 frame
class VP8Payloader {
 public:
  VP8Payloader();
  ~VP8Payloader();

  // Enables or disables the picture ID field in the VP8 RTP packets
  void EnablePictureID(bool enable);

  // Gets the current picture ID value
  uint16_t GetPictureID() const;

  // Sets the initial picture ID value
  void SetPictureID(uint16_t id);

  // Payload fragments a VP8 packet across one or more byte arrays
  // mtu is the maximum size each fragment can have
  std::vector<std::vector<uint8_t>> Payload(uint16_t mtu, const uint8_t* payload, size_t payload_size);

 private:
  bool enable_picture_id_;
  uint16_t picture_id_;
  
  static const int kVP8HeaderSize = 1;
};

#endif  // VP8_RTP_PACKETIZER_H_