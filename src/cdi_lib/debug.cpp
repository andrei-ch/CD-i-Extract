//
//  debug.cpp
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#include "debug.h"

#include "media.h"

namespace cd_i {
namespace debug {

std::string video_coding_format(uint8_t coding_info) {
  switch (coding_info & video_coding::coding_mask) {
  case coding_CLUT4:
    return "CLUT4";
  case coding_CLUT7:
    return "CLUT7";
  case coding_CLUT8:
    return "CLUT8";
  case coding_RL3:
    return "RL3";
  case coding_RL7:
    return "RL7";
  case coding_DYUV:
    return "DYUV";
  case coding_RGB555_lower:
    return "RGB555L";
  case coding_RGB555_upper:
    return "RGB555U";
  case coding_QHY:
    return "QHY";
  default:
    throw std::runtime_error("invalid video coding");
  }
}

std::string video_coding_resolution(uint8_t coding_info) {
  switch (coding_info & video_coding::resolution_mask) {
  case resolution_normal:
    return "normal";
  case resolution_double:
    return "double";
  case resolution_high:
    return "high";
  default:
    throw std::runtime_error("invalid video resolution");
  }
}

std::string audio_channel_layout(uint8_t coding_info) {
  switch (coding_info & audio_coding::channel_layout_mask) {
  case channel_layout_mono:
    return "mono";
  case channel_layout_stereo:
    return "stereo";
  default:
    throw std::runtime_error("invalid audio channels");
  }
}

std::string audio_sampling_rate(uint8_t coding_info) {
  switch (coding_info & audio_coding::sampling_rate_mask) {
  case sampling_rate_18_9kHz:
    return "18.9kHz";
  case sampling_rate_37_8kHz:
    return "37.8kHz";
  default:
    throw std::runtime_error("invalid sampling rate");
  }
}

std::string audio_bits_per_sample(uint8_t coding_info) {
  switch (coding_info & audio_coding::bits_per_sample_mask) {
  case bits_per_sample_4:
    return "4bps";
  case bits_per_sample_8:
    return "8bps";
  default:
    throw std::runtime_error("invalid bits per sample");
  }
}

void dump_sector_header(const sector_header &h) {
  printf("mode=%u "
         "filenum=%u channel_num=%u submode=%s,%s,%s,%s,%s,%s,%s,%s",
         h.mode, h.file_num, h.channel_num, (h.submode & eof) ? "EOF" : "",
         (h.submode & realtime) ? "RT" : "", (h.submode & form) ? "F" : "",
         (h.submode & trigger) ? "T" : "", (h.submode & data) ? "D" : "",
         (h.submode & audio) ? "A" : "", (h.submode & video) ? "V" : "",
         (h.submode & eor) ? "EOR" : "");
  if (h.submode & video) {
    if (h.coding_info == video_coding_MPEG) {
      printf(" coding_info=MPEG");
    } else {
      printf(" coding_info=%s,%s,%s,%s",
             video_coding_format(h.coding_info).c_str(),
             video_coding_resolution(h.coding_info).c_str(),
             h.coding_info & odd_line ? "O" : "",
             h.coding_info & app_specific ? "A" : "");
    }
  } else if (h.submode & audio) {
    if (h.coding_info == audio_coding_MPEG) {
      printf(" coding_info=MPEG");
    } else {
      printf(" coding_info=%s,%s,%s,%s",
             audio_channel_layout(h.coding_info).c_str(),
             audio_sampling_rate(h.coding_info).c_str(),
             audio_bits_per_sample(h.coding_info).c_str(),
             h.coding_info & emphasis_on ? "E" : "");
    }
  } else {
    printf(" coding_info=%u", h.coding_info);
  }
  printf("\n");
}

} // namespace debug
} // namespace cd_i
