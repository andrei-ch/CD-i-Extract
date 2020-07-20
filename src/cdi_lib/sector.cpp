//
//  sector.cpp
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#include "sector.h"
#include "parse.h"
#include "util.h"

#include <boost/format.hpp>

namespace cd_i {

bool disc_sequential_reader::fetch_next_sector(sector_data &sector) {
  if (done_) {
    throw std::runtime_error("done parsing");
  }

  if (!streamin_) {
    streamin_ = std::make_unique<std::ifstream>(path_, std::ios_base::binary);

    size_t sync_ptr = 0;

    while (*streamin_ && sync_ptr < sync_pattern.size()) {
      const uint8_t byte = streamin_->get();

      if (sync_pattern[sync_ptr] == byte) {
        ++sync_ptr;
      } else {
        sync_ptr = 0;
      }
    }

    if (sync_ptr < sync_pattern.size()) {
      return false;
    }

    std::copy(sync_pattern.begin(), sync_pattern.end(), sector.begin());

    streamin_->read(reinterpret_cast<char *>(&sector[sync_pattern.size()]),
                    sector.size() - sync_pattern.size());

    address_byte_offset_ = static_cast<uint32_t>(streamin_->tellg());
    address_byte_offset_ -= sector_size;

    sector_data sec_copy = sector;
    unscramble_sector(sec_copy);
    const sector_header &header = parse::get_sector_header(sec_copy);
    address_block_offset_ =
        util::sector_address_to_block(header.minutes, header.seconds,
                                      header.sectors) -
        150;
  } else {
    streamin_->read(reinterpret_cast<char *>(&sector[0]), sector.size());
  }

  if (streamin_->fail() ||
      !std::equal(sync_pattern.begin(), sync_pattern.end(), sector.begin())) {
    close();
    return false;
  }

  ++num_fetched_;
  return true;
}

void disc_sequential_reader::close() {
  done_ = true;
  streamin_.reset();
}

void disc_sequential_reader::seek(uint32_t block) {
  const uint64_t seek_pos =
      (block - address_block_offset_) * sector_size + address_byte_offset_;
  streamin_->seekg(seek_pos);
}

void disc_sequential_reader::unscramble_sector(sector_data &sector) const {
  assert(std::equal(sync_pattern.begin(), sync_pattern.end(), sector.begin()));

  uint16_t lfsr = 1;
  for (auto it = sector.begin() + sync_pattern.size(); it != sector.end();
       ++it) {
    uint16_t byte = *it;
    for (unsigned i = 0; i < 8; ++i) {
      byte ^= util::shift_lfsr(&lfsr) << i;
    }
    *it = static_cast<uint8_t>(byte);
  }
}

} // namespace cd_i
