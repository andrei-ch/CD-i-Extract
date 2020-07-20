//
//  util.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include "sector.h"

namespace cd_i {
namespace util {

inline uint8_t decode_address_component(uint8_t n) {
  return (n >> 4) * 10 + (n & 0x0f);
}

inline uint32_t sector_address_to_block(uint8_t minutes, uint8_t seconds,
                                        uint8_t sectors) {
  return ((decode_address_component(minutes) * 60) +
          decode_address_component(seconds) * 75) +
         sectors;
}

inline uint32_t swap_byte_order(uint32_t n) {
  return (n << 24) | ((n << 8) & 0x00ff0000) | ((n >> 8) & 0x0000ff00) |
         (n >> 24);
}

inline uint16_t shift_lfsr(uint16_t *lfsr) {
  // taps: 15 14; feedback polynomial: x^15 + x^14 + 1
  const auto ret = *lfsr & 1;
  const auto bit = ((*lfsr >> 0) ^ (*lfsr >> 1)) & 1;
  *lfsr = (*lfsr >> 1) | (bit << 14);
  return ret;
}

} // namespace util
} // namespace cd_i
