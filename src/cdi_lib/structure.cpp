//
//  structure.cpp
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#include "structure.h"
#include "parse.h"
#include "util.h"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <filesystem>

namespace cd_i {

void disc_structure_reader::discard_sectors(
    std::function<bool(const sector_data &)> predicate) {
  read_sectors(predicate);
}

void disc_structure_reader::read_sectors(
    std::function<bool(const sector_data &)> action,
    bool consume_last /*= false*/) {
  if (!has_current_sector_) {
    if (!reader().fetch_next_sector(current_sector_)) {
      has_current_sector_ = false;
      throw std::runtime_error("error reading sector");
    }
    reader().unscramble_sector(current_sector_);
    has_current_sector_ = true;
  }

  while (action(current_sector())) {
    if (!reader().fetch_next_sector(current_sector_)) {
      has_current_sector_ = false;
      throw std::runtime_error("error reading sector");
    }
    reader().unscramble_sector(current_sector_);
  }
  if (consume_last) {
    has_current_sector_ = false;
  }
}

void disc_structure_reader::init_reader() {
  if (inited_ || failed_) {
    return;
  }

  try {
    discard_sectors([](const sector_data &sector) {
      return parse::is_message_sector(sector);
    });
    read_disc_labels();
    read_path_table();
  } catch (std::exception &ex) {
    failed_ = false;
    throw;
  }

  inited_ = true;
}

std::vector<std::string> disc_structure_reader::copy_all_paths() const {
  std::vector<std::string> sorted_paths;
  for (const auto &pair : path_table()) {
    sorted_paths.push_back(pair.first);
  }
  std::sort(sorted_paths.begin(), sorted_paths.end());
  return sorted_paths;
}

void disc_structure_reader::read_disc_labels() {
  read_sectors(
      [this](const sector_data &sector) {
        if (parse::get_mode2_form1_data<disc_label>(sector)->record_type == 1) {
          disc_labels_.push_back(
              *parse::get_mode2_form1_data<disc_label>(sector));
          return true;
        }
        if (parse::get_mode2_form1_data<disc_label_terminator>(current_sector())
                ->record_type == 255) {
          return false;
        }
        throw std::runtime_error("corrupted data");
      },
      true);
}

void disc_structure_reader::read_path_table() {
  const uint32_t address =
      util::swap_byte_order(first_disc_label().path_table_address);
  reader().seek(address);

  std::vector<mode2_form1_data> raw_data;
  read_sectors(
      [&raw_data](const sector_data &sector) {
        raw_data.emplace_back();
        parse::copy_mode2_form1_data(sector, raw_data.back());
        return !parse::is_eof_sector(sector);
      },
      true);
  parse_path_table(raw_data);
}

void disc_structure_reader::parse_path_table(
    const std::vector<mode2_form1_data> &raw_data) {
  std::vector<uint8_t> data;
  for (const auto &subdata : raw_data) {
    data.insert(data.end(), subdata.begin(), subdata.end());
  }

  const size_t expected_size =
      util::swap_byte_order(first_disc_label().path_table_size);
  const uint8_t *current = &data[0];
  const uint8_t *end = current + std::min(expected_size, data.size());

  while (current < end) {
    if (current + sizeof(path_table_entry) > end) {
      throw std::runtime_error("corrupted data");
    }

    const path_table_entry *entry =
        reinterpret_cast<const path_table_entry *>(current);

    const uint8_t *next = current + sizeof(path_table_entry) + entry->name_len +
                          ((entry->name_len & 1) ? 1 : 0);
    if (next > end) {
      throw std::runtime_error("corrupted data");
    }

    std::string name;
    if (entry->name_len == 1 && entry->name[0] == 0) {
      name = ".";
    } else {
      name.append(entry->name, entry->name_len);
    }
    path_table_.emplace(name, *entry);

    current = next;
  }
}

void disc_structure_reader::seek(const directory_entry &entry) {
  const uint32_t address = util::swap_byte_order(entry.file_address);
  reader().seek(address);
  has_current_sector_ = false;
}

void disc_structure_reader::seek(const path_table_entry &entry) {
  const uint32_t address = util::swap_byte_order(entry.directory_address);
  reader().seek(address);
  has_current_sector_ = false;
}

bool disc_structure_reader::read_directory(std::string path,
                                           directory_entry_handler handler) {
  const auto found = path_table_.find(path);
  if (found == path_table_.end()) {
    return false;
  }

  seek(found->second);

  std::vector<mode2_form1_data> raw_data;

  read_sectors(
      [&raw_data](const sector_data &sector) {
        raw_data.emplace_back();
        parse::copy_mode2_form1_data(sector, raw_data.back());
        return !parse::is_eof_sector(sector);
      },
      true);

  parse_directory(raw_data, handler);
  return true;
}

bool disc_structure_reader::read_directory(
    std::string path,
    std::unordered_map<std::string, directory_entry_2> &files) {
  files.clear();
  return read_directory(path,
                        [&files](std::string name, const directory_entry &entry,
                                 const directory_entry_ex &entry_ex) {
                          files.emplace(name, std::make_pair(entry, entry_ex));
                          return true;
                        });
}

void disc_structure_reader::parse_directory(
    const std::vector<mode2_form1_data> &raw_data,
    directory_entry_handler handler) {
  for (const auto &data : raw_data) {
    const uint8_t *current = &data[0];
    const uint8_t *end = current + data.size();
    while (current < end) {
      const directory_entry *entry =
          reinterpret_cast<const directory_entry *>(current);
      const char *entry_name =
          reinterpret_cast<const char *>(current + sizeof(directory_entry));

      static_assert(sizeof(entry->entry_len) == 1, "");
      if (entry->entry_len == 0) {
        break;
      }

      const uint8_t *next = current + entry->entry_len;
      if (next > end) {
        throw std::runtime_error("corrupted data");
      }

      const size_t entry_ex_offset =
          sizeof(*entry) + entry->name_len + ((entry->name_len & 1) ? 0 : 1);

      const directory_entry_ex *entry_ex =
          reinterpret_cast<const directory_entry_ex *>(current +
                                                       entry_ex_offset);

      std::string name;
      size_t name_len = entry->name_len;
      if (name_len == 1 && entry_name[0] == 0) {
        name = ".";
      } else if (name_len == 1 && entry_name[0] == 1) {
        name = "..";
      } else {
        if (name_len >= 3 && entry_name[name_len - 2] == ';' &&
            entry_name[name_len - 1] == '1') {
          name_len -= 2;
        }
        name.append(entry_name, name_len);
      }

      if (!handler(name, *entry, *entry_ex)) {
        return;
      }

      current = next;
    }
  }
}

bool disc_structure_reader::read_file(const directory_entry &entry,
                                      const directory_entry_ex &entry_ex,
                                      file_handler handler) {
  const uint8_t file_num = entry_ex.file_number;
  size_t remaining =
      static_cast<size_t>(util::swap_byte_order(entry.file_size));

  seek(entry);
  read_sectors([&](const sector_data &sector) {
    if (file_num && parse::get_sector_header(sector).file_num != file_num) {
      return true;
    }
    if (parse::is_mode2_form1_sector(sector)) {
      const size_t size = std::min(remaining, mode2_form1_data_size);
      if (!handler(parse::get_mode2_form1_data<char>(sector), size)) {
        return false;
      }
      remaining -= size;
    } else if (parse::is_mode2_form2_sector(sector)) {
      const size_t size = std::min(remaining, mode2_form2_data_size);
      if (!handler(parse::get_mode2_form2_data<char>(sector), size)) {
        return false;
      }
      remaining -= size;
    } else {
      throw std::runtime_error("corrupted data");
    }
    return remaining > 0;
  });
  return true;
}

bool disc_structure_reader::scan_file(const directory_entry &entry,
                                      const directory_entry_ex &entry_ex,
                                      scan_handler handler) {
  const uint8_t file_num = entry_ex.file_number;
  size_t remaining =
      static_cast<size_t>(util::swap_byte_order(entry.file_size));

  seek(entry);
  read_sectors([&](const sector_data &sector) {
    if (file_num && parse::get_sector_header(sector).file_num != file_num) {
      return true;
    }
    if (parse::is_mode2_form1_sector(sector)) {
      const size_t size = std::min(remaining, mode2_form1_data_size);
      remaining -= size;
    } else if (parse::is_mode2_form2_sector(sector)) {
      const size_t size = std::min(remaining, mode2_form2_data_size);
      remaining -= size;
    } else {
      throw std::runtime_error("corrupted data");
    }
    if (!handler(sector)) {
      return false;
    }
    return remaining > 0;
  });
  return true;
}

bool disc_structure_reader::stat_file(std::string directory_path,
                                      std::string filename,
                                      directory_entry &entry,
                                      directory_entry_ex &entry_ex) {
  bool found = false;
  if (!read_directory(directory_path,
                      [&](std::string name, const directory_entry &found_entry,
                          const directory_entry_ex &found_entry_ex) {
                        if (name == filename) {
                          entry = found_entry;
                          entry_ex = found_entry_ex;
                          found = true;
                          return false;
                        }
                        return true;
                      })) {
    return false;
  }
  return found;
}

bool disc_structure_reader::read_file(std::string directory_path,
                                      std::string filename,
                                      file_handler handler) {
  directory_entry entry;
  directory_entry_ex entry_ex;
  if (!stat_file(directory_path, filename, entry, entry_ex)) {
    return false;
  }
  return read_file(entry, entry_ex, handler);
}

bool disc_structure_reader::copy_file(const directory_entry &entry,
                                      const directory_entry_ex &entry_ex,
                                      std::string destination) {
  try {
    std::ofstream stream_out(destination);
    if (!read_file(entry, entry_ex, [&](const char *data, size_t size) {
          stream_out.write(data, size);
          return true;
        })) {
      throw std::runtime_error("file not found");
    }
  } catch (std::exception &ex) {
    boost::filesystem::remove(destination);
    return false;
  }
  return true;
}

bool disc_structure_reader::copy_file(std::string directory_path,
                                      std::string filename,
                                      std::string destination) {
  directory_entry entry;
  directory_entry_ex entry_ex;
  if (!stat_file(directory_path, filename, entry, entry_ex)) {
    return false;
  }
  return copy_file(entry, entry_ex, destination);
}

} // namespace cd_i
