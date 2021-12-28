//
//  structure.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include "sector.h"

#include <functional>
#include <unordered_map>
#include <vector>

namespace cd_i {

struct __attribute__((packed)) disc_label {
  uint8_t record_type;
  char volume_structure_standard_id[5];
  uint8_t volume_structure_version;
  uint8_t volume_flags;
  char system_id[32];
  char volume_id[32];
  uint8_t reserved_0[12];
  uint32_t volume_space_size;
  char charset_id[32];
  uint8_t reserved_1[2];
  uint16_t num_volumes;
  uint8_t reserved_2[2];
  uint16_t volume_seq_num;
  uint8_t reserved_3[2];
  uint16_t logical_block_size;
  uint8_t reserved_4[4];
  uint32_t path_table_size;
  uint8_t reserved_5[8];
  uint32_t path_table_address;
  uint8_t reserved_6[38];
  char album[128];
  char publisher[128];
  char preparer[128];
  char application[128];
  char copyright[32];
  uint8_t reserved_7[5];
  char abstract_file_name[32];
  uint8_t reserved_8[5];
  char bibliographic_file_name[32];
  uint8_t reserved_9[5];
  char creation_datetime[16];
  uint8_t reserved_10[1];
  char modification_datetime[16];
  uint8_t reserved_11[1];
  char expiration_datetime[16];
  uint8_t reserved_12[1];
  char effective_datetime[16];
  uint8_t reserved_13[1];
  uint8_t file_structure_standard_version;
  uint8_t reserved_14[1];
  char application_data[512];
  uint8_t reserved_15[653];
};

static_assert(sizeof(disc_label) == mode2_form1_data_size, "");

struct __attribute__((packed)) disc_label_terminator {
  uint8_t record_type;
  char volume_structure_standard_id[5];
  uint8_t volume_structure_version;
  uint8_t reserved_0[2041];
};

static_assert(sizeof(disc_label_terminator) == mode2_form1_data_size, "");

struct __attribute__((packed)) path_table_entry {
  uint8_t name_len;
  uint8_t ext_attr_len;
  uint32_t directory_address;
  uint16_t parent_directory_number;
  char name[];
};

struct __attribute__((packed)) directory_entry {
  uint8_t entry_len;
  uint8_t ext_attr_len;
  uint8_t reserved_0[4];
  uint32_t file_address;
  uint8_t reserved_1[4];
  uint32_t file_size;
  uint8_t creation_date[6];
  uint8_t reserved_2[1];
  uint8_t file_flags;
  uint8_t interleave[2];
  uint8_t reserved_3[2];
  uint16_t volume_seq_num;
  uint8_t name_len;
};

struct __attribute__((packed)) directory_entry_ex {
  uint32_t owner_id;
  uint16_t file_attr;
  uint8_t reserved_0[2];
  uint8_t file_number;
  uint8_t reserved_1[1];
};

enum file_attr {
  world_read = 1 << 0,
  world_execute = 1 << 2,
  cdda = 1 << 6,
  directory = 1 << 7,
  owner_read = 1 << 8,
  owner_execute = 1 << 10,
  group_read = 1 << 12,
  group_execute = 1 << 14,
};

using directory_entry_2 = std::pair<directory_entry, directory_entry_ex>;

class disc_structure_reader {
public:
  disc_structure_reader(std::string path);

  void init_reader();

  const std::unordered_map<std::string, path_table_entry> &path_table() const;
  std::vector<std::string> copy_all_paths() const;

  const disc_label &first_disc_label() const;

  using directory_entry_handler = std::function<bool(
      std::string, const directory_entry &, const directory_entry_ex &)>;

  bool read_directory(std::string path, directory_entry_handler handler);
  bool
  read_directory(std::string path,
                 std::unordered_map<std::string, directory_entry_2> &files);

  bool stat_file(std::string directory_path, std::string filename,
                 directory_entry &entry, directory_entry_ex &entry_ex);

  using file_handler = std::function<bool(const char *, size_t)>;

  bool read_file(const directory_entry &entry,
                 const directory_entry_ex &entry_ex, file_handler handler);
  bool read_file(std::string directory_path, std::string filename,
                 file_handler handler);

  bool copy_file(const directory_entry &entry,
                 const directory_entry_ex &entry_ex, std::string destination);
  bool copy_file(std::string directory_path, std::string filename,
                 std::string destination);

  using scan_handler = std::function<bool(const sector_data &)>;

  bool scan_file(const directory_entry &entry,
                 const directory_entry_ex &entry_ex, scan_handler handler);

protected:
  void seek(const directory_entry &entry);
  void seek(const directory_entry_2 &entry);
  void seek(const path_table_entry &entry);
  void read_sectors(std::function<bool(const sector_data &)> action,
                    bool consume_last = false);
  void discard_sectors(std::function<bool(const sector_data &)> predicate);

private:
  disc_sequential_reader &reader();
  void read_disc_labels();
  void read_path_table();
  void parse_path_table(const std::vector<mode2_form1_data> &raw_data);
  void parse_directory(const std::vector<mode2_form1_data> &raw_data,
                       directory_entry_handler handler);
  const sector_data &current_sector() const;

private:
  disc_sequential_reader reader_;
  bool inited_ = false;
  bool failed_ = false;
  bool has_current_sector_ = false;
  sector_data current_sector_;
  std::vector<disc_label> disc_labels_;
  std::unordered_map<std::string, path_table_entry> path_table_;
};

inline disc_structure_reader::disc_structure_reader(std::string path)
    : reader_(path) {}

inline const std::unordered_map<std::string, path_table_entry> &
disc_structure_reader::path_table() const {
  return path_table_;
}

inline disc_sequential_reader &disc_structure_reader::reader() {
  return reader_;
}

inline const sector_data &disc_structure_reader::current_sector() const {
  return current_sector_;
}

inline const disc_label &disc_structure_reader::first_disc_label() const {
  return disc_labels_.front();
}

inline void disc_structure_reader::seek(const directory_entry_2 &entry) {
  seek(entry.first);
}

} // namespace cd_i
