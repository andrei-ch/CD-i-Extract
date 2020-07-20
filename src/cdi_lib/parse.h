//
//  parse.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include "media.h"
#include "structure.h"

namespace cd_i {
namespace parse {

inline bool is_valid_sector(const sector_data &sector) {
  return std::equal(sync_pattern.begin(), sync_pattern.end(), sector.begin());
}

inline bool is_unscrambled_sector(const sector_data &sector) {
  return is_valid_sector(sector) &&
         (sector[15] == sector_mode_1 || sector[15] == sector_mode_2);
}

inline const sector_header &get_sector_header(const sector_data &sector) {
  assert(is_unscrambled_sector(sector));
  return *reinterpret_cast<const sector_header *>(&sector[12]);
}

inline bool is_mode1_sector(const sector_header &header) {
  assert(header.mode == sector_mode_1 || header.mode == sector_mode_2);
  return header.mode == sector_mode_1;
}

inline bool is_mode1_sector(const sector_data &sector) {
  return is_mode1_sector(get_sector_header(sector));
}

inline bool is_mode2_sector(const sector_header &header) {
  assert(header.mode == sector_mode_1 || header.mode == sector_mode_2);
  return header.mode == sector_mode_2;
}

inline bool is_mode2_sector(const sector_data &sector) {
  return is_mode2_sector(get_sector_header(sector));
}

inline bool is_mode2_form1_sector(const sector_header &header) {
  return is_mode2_sector(header) && !(header.submode & submode::form);
}

inline bool is_mode2_form1_sector(const sector_data &sector) {
  return is_mode2_form1_sector(get_sector_header(sector));
}

inline bool is_mode2_form2_sector(const sector_header &header) {
  assert(header.mode == sector_mode_1 || header.mode == sector_mode_2);
  return is_mode2_sector(header) && (header.submode & submode::form);
}

inline bool is_mode2_form2_sector(const sector_data &sector) {
  return is_mode2_form2_sector(get_sector_header(sector));
}

inline bool is_eof_sector(const sector_header &header) {
  return is_mode2_sector(header) && (header.submode & submode::eof);
}

inline bool is_eof_sector(const sector_data &sector) {
  return is_eof_sector(get_sector_header(sector));
}

inline bool is_message_sector(const sector_header &header) {
  return is_mode2_form2_sector(header) && header.file_num == 0 &&
         header.channel_num == 0 && header.coding_info == 0;
}

inline bool is_message_sector(const sector_data &sector) {
  return is_message_sector(get_sector_header(sector));
}

inline bool is_empty_sector(const sector_header &header) {
  return is_mode2_sector(header) && header.channel_num == 0 &&
         (header.submode & (submode::video | submode::audio | submode::data)) ==
             0 &&
         header.coding_info == 0;
}

inline bool is_empty_sector(const sector_data &sector) {
  return is_empty_sector(get_sector_header(sector));
}

inline bool is_audio_sector(const sector_header &header) {
  return is_mode2_sector(header) && (header.submode & submode::audio) != 0;
}

inline bool is_audio_sector(const sector_data &sector) {
  return is_audio_sector(get_sector_header(sector));
}

inline bool is_mpeg_audio_sector(const sector_header &header) {
  return is_audio_sector(header) &&
         header.coding_info == audio_coding::audio_coding_MPEG;
}

inline bool is_mpeg_audio_sector(const sector_data &sector) {
  return is_mpeg_audio_sector(get_sector_header(sector));
}

inline bool is_video_sector(const sector_header &header) {
  return is_mode2_sector(header) && (header.submode & submode::video) != 0;
}

inline bool is_video_sector(const sector_data &sector) {
  return is_video_sector(get_sector_header(sector));
}

inline bool is_mpeg_video_sector(const sector_header &header) {
  return is_video_sector(header) &&
         header.coding_info == video_coding::video_coding_MPEG;
}

inline bool is_mpeg_video_sector(const sector_data &sector) {
  return is_mpeg_video_sector(get_sector_header(sector));
}

template <typename T>
inline const T *get_mode1_data(const sector_data &sector) {
  assert(is_mode1_sector(sector));
  return reinterpret_cast<const T *>(&sector[mode1_data_offset]);
}

template <typename T>
inline const T *get_mode2_data(const sector_data &sector) {
  assert(is_mode2_sector(sector));
  return reinterpret_cast<const T *>(&sector[mode2_data_offset]);
}

template <typename T>
inline const T *get_mode2_form1_data(const sector_data &sector) {
  assert(is_mode2_form1_sector(sector));
  return reinterpret_cast<const T *>(&sector[mode2_form1_data_offset]);
}

template <typename T>
inline const T *get_mode2_form2_data(const sector_data &sector) {
  assert(is_mode2_form2_sector(sector));
  return reinterpret_cast<const T *>(&sector[mode2_form2_data_offset]);
}

inline void copy_mode1_data(const sector_data &sector, mode1_data &data) {
  assert(is_mode1_sector(sector));
  const auto begin = sector.begin() + mode1_data_offset;
  std::copy(begin, begin + data.size(), data.begin());
}

inline void copy_mode2_data(const sector_data &sector, mode2_data &data) {
  assert(is_mode2_sector(sector));
  const auto begin = sector.begin() + mode2_data_offset;
  std::copy(begin, begin + data.size(), data.begin());
}

inline void copy_mode2_form1_data(const sector_data &sector,
                                  mode2_form1_data &data) {
  assert(is_mode2_form1_sector(sector));
  const auto begin = sector.begin() + mode2_form1_data_offset;
  std::copy(begin, begin + data.size(), data.begin());
}

inline void copy_mode2_form2_data(const sector_data &sector,
                                  mode2_form2_data &data) {
  assert(is_mode2_form2_sector(sector));
  const auto begin = sector.begin() + mode2_form2_data_offset;
  std::copy(begin, begin + data.size(), data.begin());
}

inline std::string copy_disc_label(const disc_label &label) {
  std::string str;
  str.append(label.volume_id, label.volume_id + sizeof(label.volume_id));
  str.erase(str.find_last_not_of(" ") + 1);
  return str;
}

inline bool is_directory(const directory_entry_ex &entry) {
  return entry.file_attr & file_attr::directory;
}

inline bool is_directory(const directory_entry_2 &entry) {
  return is_directory(entry.second);
}

} // namespace parse
} // namespace cd_i
