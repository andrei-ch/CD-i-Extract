//
//  debug.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include "sector.h"

namespace cd_i {
namespace debug {

std::string video_coding_format(uint8_t coding_info);
std::string video_coding_resolution(uint8_t coding_info);
std::string audio_channel_layout(uint8_t coding_info);
std::string audio_sampling_rate(uint8_t coding_info);
std::string audio_bits_per_sample(uint8_t coding_info);
void dump_sector_header(const sector_header &h);

} // namespace debug
} // namespace cd_i
