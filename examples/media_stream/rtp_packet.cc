#include "rtp_packet.h"

#include <cstring>
#include <algorithm>

RTPPacket::RTPPacket()
    : version_(0),
      padding_(false),
      extension_(false),
      csrc_count_(0),
      marker_(false),
      payload_type_(0),
      sequence_number_(0),
      timestamp_(0),
      ssrc_(0),
      extension_header_id_(0),
      extension_length_(0),
      extension_value_(nullptr),
      padding_size_(0),
      payload_(nullptr),
      payload_size_(0) {}

RTPPacket::~RTPPacket() {
  delete[] extension_value_;
  delete[] payload_;
}

bool RTPPacket::Unmarshal(const uint8_t* buffer, size_t buffer_size) {
  // Clean up any existing data
  delete[] extension_value_;
  extension_value_ = nullptr;
  
  delete[] payload_;
  payload_ = nullptr;
  payload_size_ = 0;

  // Check minimum packet size (RTP header is at least 12 bytes)
  if (buffer_size < 12) {
    return false;
  }

  // Parse header fields
  version_ = (buffer[0] >> 6) & 0x03;
  padding_ = (buffer[0] >> 5) & 0x01;
  extension_ = (buffer[0] >> 4) & 0x01;
  csrc_count_ = buffer[0] & 0x0F;
  
  marker_ = (buffer[1] >> 7) & 0x01;
  payload_type_ = buffer[1] & 0x7F;
  
  sequence_number_ = (static_cast<uint16_t>(buffer[2]) << 8) | buffer[3];
  
  timestamp_ = (static_cast<uint32_t>(buffer[4]) << 24) |
              (static_cast<uint32_t>(buffer[5]) << 16) |
              (static_cast<uint32_t>(buffer[6]) << 8) |
              buffer[7];
  
  ssrc_ = (static_cast<uint32_t>(buffer[8]) << 24) |
         (static_cast<uint32_t>(buffer[9]) << 16) |
         (static_cast<uint32_t>(buffer[10]) << 8) |
         buffer[11];

  // Validate that the packet is large enough to contain the CSRC list
  size_t header_size = 12 + (csrc_count_ * 4);
  if (buffer_size < header_size) {
    return false;
  }

  // Extract CSRC list
  csrcs_.clear();
  for (int i = 0; i < csrc_count_; i++) {
    uint32_t csrc = (static_cast<uint32_t>(buffer[12 + (i * 4)]) << 24) |
                   (static_cast<uint32_t>(buffer[13 + (i * 4)]) << 16) |
                   (static_cast<uint32_t>(buffer[14 + (i * 4)]) << 8) |
                   buffer[15 + (i * 4)];
    csrcs_.push_back(csrc);
  }

  // Handle header extension if present
  if (extension_) {
    // Check if packet is large enough to contain the extension header
    if (buffer_size < header_size + 4) {
      return false;
    }

    extension_header_id_ = (static_cast<uint16_t>(buffer[header_size]) << 8) | 
                          buffer[header_size + 1];
    
    extension_length_ = (static_cast<uint16_t>(buffer[header_size + 2]) << 8) | 
                       buffer[header_size + 3];
    
    // Extension length is in 32-bit (4-byte) words
    size_t extension_size = extension_length_ * 4;
    
    // Check if packet is large enough to contain the extension data
    if (buffer_size < header_size + 4 + extension_size) {
      return false;
    }

    // Store extension value
    extension_value_ = new uint8_t[extension_size];
    std::memcpy(extension_value_, buffer + header_size + 4, extension_size);
    
    // Update header size to include extension
    header_size += 4 + extension_size;
  }

  // Calculate payload size considering padding
  size_t payload_size = buffer_size - header_size;
  
  if (padding_ && payload_size > 0) {
    padding_size_ = buffer[buffer_size - 1];
    
    // Validate padding size
    if (padding_size_ == 0 || padding_size_ > payload_size) {
      return false;
    }
    
    payload_size -= padding_size_;
  } else {
    padding_size_ = 0;
  }

  // Extract payload
  if (payload_size > 0) {
    payload_ = new uint8_t[payload_size];
    std::memcpy(payload_, buffer + header_size, payload_size);
    payload_size_ = payload_size;
  }

  return true;
}