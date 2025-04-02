#ifndef VP8_PACKET_H_
#define VP8_PACKET_H_

#include <cstdint>
#include <vector>

// Error codes
enum VP8PacketError {
  VP8_PACKET_OK = 0,
  VP8_PACKET_ERROR_NIL_PACKET,
  VP8_PACKET_ERROR_SHORT_PACKET,
};

// VP8Packet represents the VP8 header that is stored in the payload of an RTP Packet
class VP8Packet {
 public:
  VP8Packet();
  
  // Unmarshal parses the passed byte slice and stores the result in the VP8Packet
  // Returns the payload and error code
  VP8PacketError Unmarshal(const uint8_t* packet, size_t packet_size, 
                         std::vector<uint8_t>* payload);
  
  // IsPartitionHead checks whether if this is a head of the VP8 partition
  static bool IsPartitionHead(const uint8_t* payload, size_t payload_size);

  // Required Header
  uint8_t X() const { return x_; }   // extended control bits present
  uint8_t N() const { return n_; }   // when set to 1 this frame can be discarded
  uint8_t S() const { return s_; }   // start of VP8 partition
  uint8_t PID() const { return pid_; }  // partition index

  // Extended control bits
  uint8_t I() const { return i_; }   // 1 if PictureID is present
  uint8_t L() const { return l_; }   // 1 if TL0PICIDX is present
  uint8_t T() const { return t_; }   // 1 if TID is present
  uint8_t K() const { return k_; }   // 1 if KEYIDX is present

  // Optional extension
  uint16_t PictureID() const { return picture_id_; }  // 8 or 16 bits, picture ID
  uint8_t TL0PICIDX() const { return tl0pic_idx_; }   // 8 bits temporal level zero index
  uint8_t TID() const { return tid_; }               // 2 bits temporal layer index
  uint8_t Y() const { return y_; }                   // 1 bit layer sync bit
  uint8_t KEYIDX() const { return key_idx_; }        // 5 bits temporal key frame index

 private:
  // Required Header
  uint8_t x_;   // extended control bits present
  uint8_t n_;   // when set to 1 this frame can be discarded
  uint8_t s_;   // start of VP8 partition
  uint8_t pid_; // partition index

  // Extended control bits
  uint8_t i_;   // 1 if PictureID is present
  uint8_t l_;   // 1 if TL0PICIDX is present
  uint8_t t_;   // 1 if TID is present
  uint8_t k_;   // 1 if KEYIDX is present

  // Optional extension
  uint16_t picture_id_;  // 8 or 16 bits, picture ID
  uint8_t tl0pic_idx_;   // 8 bits temporal level zero index
  uint8_t tid_;          // 2 bits temporal layer index
  uint8_t y_;            // 1 bit layer sync bit
  uint8_t key_idx_;      // 5 bits temporal key frame index
};

#endif  // VP8_PACKET_H_