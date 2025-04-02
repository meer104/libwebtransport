#include "vp8_packet.h"

#include <cstring>

VP8Packet::VP8Packet()
    : x_(0),
      n_(0),
      s_(0),
      pid_(0),
      i_(0),
      l_(0),
      t_(0),
      k_(0),
      picture_id_(0),
      tl0pic_idx_(0),
      tid_(0),
      y_(0),
      key_idx_(0) {}

VP8PacketError VP8Packet::Unmarshal(const uint8_t* payload, size_t payload_size, 
                                  std::vector<uint8_t>* output_payload) {
  if (!payload || payload_size == 0) {
    return VP8_PACKET_ERROR_NIL_PACKET;
  }

  size_t payload_index = 0;

  if (payload_index >= payload_size) {
    return VP8_PACKET_ERROR_SHORT_PACKET;
  }

  // Parse first byte
  x_ = (payload[payload_index] & 0x80) >> 7;
  n_ = (payload[payload_index] & 0x20) >> 5;
  s_ = (payload[payload_index] & 0x10) >> 4;
  pid_ = payload[payload_index] & 0x07;

  payload_index++;

  // Parse X byte if present
  if (x_ == 1) {
    if (payload_index >= payload_size) {
      return VP8_PACKET_ERROR_SHORT_PACKET;
    }
    i_ = (payload[payload_index] & 0x80) >> 7;
    l_ = (payload[payload_index] & 0x40) >> 6;
    t_ = (payload[payload_index] & 0x20) >> 5;
    k_ = (payload[payload_index] & 0x10) >> 4;
    payload_index++;
  } else {
    i_ = 0;
    l_ = 0;
    t_ = 0;
    k_ = 0;
  }

  // Parse PictureID if present
  if (i_ == 1) {
    if (payload_index >= payload_size) {
      return VP8_PACKET_ERROR_SHORT_PACKET;
    }
    if (payload[payload_index] & 0x80) {  // M == 1, PID is 16bit
      if (payload_index + 1 >= payload_size) {
        return VP8_PACKET_ERROR_SHORT_PACKET;
      }
      picture_id_ = static_cast<uint16_t>((payload[payload_index] & 0x7F) << 8 | 
                                          payload[payload_index + 1]);
      payload_index += 2;
    } else {
      picture_id_ = payload[payload_index];
      payload_index++;
    }
  } else {
    picture_id_ = 0;
  }

  // Parse TL0PICIDX if present
  if (l_ == 1) {
    if (payload_index >= payload_size) {
      return VP8_PACKET_ERROR_SHORT_PACKET;
    }
    tl0pic_idx_ = payload[payload_index];
    payload_index++;
  } else {
    tl0pic_idx_ = 0;
  }

  // Parse TID/KEYIDX if present
  if (t_ == 1 || k_ == 1) {
    if (payload_index >= payload_size) {
      return VP8_PACKET_ERROR_SHORT_PACKET;
    }
    if (t_ == 1) {
      tid_ = payload[payload_index] >> 6;
      y_ = (payload[payload_index] >> 5) & 0x1;
    } else {
      tid_ = 0;
      y_ = 0;
    }
    if (k_ == 1) {
      key_idx_ = payload[payload_index] & 0x1F;
    } else {
      key_idx_ = 0;
    }
    payload_index++;
  } else {
    tid_ = 0;
    y_ = 0;
    key_idx_ = 0;
  }

  // Set the output payload
  if (output_payload) {
    output_payload->resize(payload_size - payload_index);
    if (!output_payload->empty()) {
      std::memcpy(output_payload->data(), payload + payload_index, payload_size - payload_index);
    }
  }

  return VP8_PACKET_OK;
}

bool VP8Packet::IsPartitionHead(const uint8_t* payload, size_t payload_size) {
  if (payload_size < 1) {
    return false;
  }
  
  return (payload[0] & 0x10) != 0;
}