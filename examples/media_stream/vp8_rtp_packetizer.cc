#include "vp8_rtp_packetizer.h"

#include <algorithm>
#include <cstring>

VP8Payloader::VP8Payloader() 
    : enable_picture_id_(false),
      picture_id_(0) {}

VP8Payloader::~VP8Payloader() {}

void VP8Payloader::EnablePictureID(bool enable) {
  enable_picture_id_ = enable;
}

uint16_t VP8Payloader::GetPictureID() const {
  return picture_id_;
}

void VP8Payloader::SetPictureID(uint16_t id) {
  picture_id_ = id & 0x7FFF;  // Ensure it fits in 15 bits
}

std::vector<std::vector<uint8_t>> VP8Payloader::Payload(uint16_t mtu, 
                                                      const uint8_t* payload,
                                                      size_t payload_size) {
  /*
   * https://tools.ietf.org/html/rfc7741#section-4.2
   *
   *       0 1 2 3 4 5 6 7
   *      +-+-+-+-+-+-+-+-+
   *      |X|R|N|S|R| PID | (REQUIRED)
   *      +-+-+-+-+-+-+-+-+
   * X:   |I|L|T|K| RSV   | (OPTIONAL)
   *      +-+-+-+-+-+-+-+-+
   * I:   |M| PictureID   | (OPTIONAL)
   *      +-+-+-+-+-+-+-+-+
   * L:   |   TL0PICIDX   | (OPTIONAL)
   *      +-+-+-+-+-+-+-+-+
   * T/K: |TID|Y| KEYIDX  | (OPTIONAL)
   *      +-+-+-+-+-+-+-+-+
   *  S: Start of VP8 partition.  SHOULD be set to 1 when the first payload
   *     octet of the RTP packet is the beginning of a new VP8 partition,
   *     and MUST NOT be 1 otherwise.  The S bit MUST be set to 1 for the
   *     first packet of each encoded frame.
   */

  std::vector<std::vector<uint8_t>> payloads;
  
  if (payload == nullptr || payload_size == 0) {
    return payloads;
  }

  int using_header_size = kVP8HeaderSize;
  if (enable_picture_id_) {
    if (picture_id_ == 0) {
      // No additional bytes needed if picture_id is 0
    } else if (picture_id_ < 128) {
      using_header_size = kVP8HeaderSize + 2;
    } else {
      using_header_size = kVP8HeaderSize + 3;
    }
  }

  int max_fragment_size = static_cast<int>(mtu) - using_header_size;
  
  // Check if the maximum fragment size is valid
  if (max_fragment_size <= 0) {
    return payloads;
  }
  
  size_t payload_remaining = payload_size;
  size_t payload_index = 0;
  bool first = true;
  
  while (payload_remaining > 0) {
    int current_fragment_size = std::min(static_cast<size_t>(max_fragment_size), payload_remaining);
    std::vector<uint8_t> out(using_header_size + current_fragment_size);
    
    // Setup basic header byte
    if (first) {
      out[0] = 0x10;  // Set S bit to 1 for first packet
      first = false;
    } else {
      out[0] = 0x00;  // No special flags for continuation packets
    }
    
    // Add picture ID if enabled
    if (enable_picture_id_) {
      switch (using_header_size) {
        case kVP8HeaderSize:
          // No picture ID field
          break;
        case kVP8HeaderSize + 2:
          out[0] |= 0x80;  // Set X bit
          out[1] = 0x80;   // Set I bit
          out[2] = static_cast<uint8_t>(picture_id_ & 0x7F);
          break;
        case kVP8HeaderSize + 3:
          out[0] |= 0x80;  // Set X bit
          out[1] = 0x80;   // Set I bit
          out[2] = 0x80 | static_cast<uint8_t>((picture_id_ >> 8) & 0x7F);
          out[3] = static_cast<uint8_t>(picture_id_ & 0xFF);
          break;
      }
    }
    
    // Copy payload fragment
    std::memcpy(out.data() + using_header_size, 
                payload + payload_index, 
                current_fragment_size);
    
    payloads.push_back(std::move(out));
    
    payload_remaining -= current_fragment_size;
    payload_index += current_fragment_size;
  }
  
  // Increment picture ID for next frame, wrapping at 0x7FFF
  picture_id_ = (picture_id_ + 1) & 0x7FFF;
  
  return payloads;
}