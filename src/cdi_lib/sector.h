//
//  sector.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace cd_i {

constexpr size_t sector_size = 2352;

constexpr std::array<uint8_t, 12> sync_pattern = {
    {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00}};

using sector_data = std::array<uint8_t, sector_size>;

enum submode {
  eor = 1 << 0,
  video = 1 << 1,
  audio = 1 << 2,
  data = 1 << 3,
  trigger = 1 << 4,
  form = 1 << 5,
  realtime = 1 << 6,
  eof = 1 << 7,
};

enum sector_mode {
  sector_mode_1 = 1,
  sector_mode_2 = 2,
};

struct __attribute__((packed)) sector_header {
  uint8_t minutes;
  uint8_t seconds;
  uint8_t sectors;
  uint8_t mode;
  uint8_t file_num;
  uint8_t channel_num;
  uint8_t submode;
  uint8_t coding_info;
};

static_assert(sizeof(sector_header) == 8, "");

constexpr size_t mode1_data_size = 2048;
constexpr size_t mode2_data_size = 2336;
constexpr size_t mode2_form1_data_size = 2048;
constexpr size_t mode2_form2_data_size = 2324;

constexpr size_t mode1_data_offset = 16;
constexpr size_t mode2_data_offset = 16;
constexpr size_t mode2_form1_data_offset = 24;
constexpr size_t mode2_form2_data_offset = 24;

using mode1_data = std::array<uint8_t, mode1_data_size>;
using mode2_data = std::array<uint8_t, mode2_data_size>;
using mode2_form1_data = std::array<uint8_t, mode2_form1_data_size>;
using mode2_form2_data = std::array<uint8_t, mode2_form2_data_size>;

class disc_sequential_reader {
public:
  disc_sequential_reader(std::string path);

  bool fetch_next_sector(sector_data &sector);
  unsigned int num_fetched_sectors() const;

  void seek(uint32_t block);

  void unscramble_sector(sector_data &sector) const;

private:
  void close();

private:
  std::string path_;
  std::unique_ptr<std::ifstream> streamin_;
  bool done_ = false;
  unsigned int num_fetched_ = 0;
  uint32_t address_byte_offset_ = 0;
  uint32_t address_block_offset_ = 0;
};

inline disc_sequential_reader::disc_sequential_reader(std::string path)
    : path_(path) {}

inline unsigned int disc_sequential_reader::num_fetched_sectors() const {
  return num_fetched_;
}

} // namespace cd_i
