//
//  video.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

namespace cd_i {

enum video_coding {
  coding_mask = 0x0f,
  coding_CLUT4 = 0x00,
  coding_CLUT7 = 0x01,
  coding_CLUT8 = 0x02,
  coding_RL3 = 0x03,
  coding_RL7 = 0x04,
  coding_DYUV = 0x05,
  coding_RGB555_lower = 0x06,
  coding_RGB555_upper = 0x07,
  coding_QHY = 0x08,

  resolution_mask = 0x30,
  resolution_normal = 0x00,
  resolution_double = 0x10,
  resolution_high = 0x30,

  odd_line = 0x40,
  app_specific = 0x80,

  video_coding_MPEG = 0x0f,
};

enum audio_coding {
  channel_layout_mask = 0x03,
  channel_layout_mono = 0x00,
  channel_layout_stereo = 0x01,

  sampling_rate_mask = 0x0c,
  sampling_rate_37_8kHz = 0x00,
  sampling_rate_18_9kHz = 0x04,

  bits_per_sample_mask = 0x30,
  bits_per_sample_4 = 0x00,
  bits_per_sample_8 = 0x10,

  emphasis_on = 0x40,

  audio_coding_MPEG = 0x7f,
};

} // namespace cd_i
