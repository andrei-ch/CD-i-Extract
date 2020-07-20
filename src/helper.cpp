//
//  helper.cpp
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#include "helper.h"

#include <boost/format.hpp>

using namespace cd_i;

namespace fs = boost::filesystem;

void cdi_helper::read_disc_paths() {
  reader_.init_reader();

  const auto disc_label = parse::copy_disc_label(reader_.first_disc_label());
  if (!disc_label.empty()) {
    std::cout << "Disc label is " << disc_label << std::endl;
  }

  paths_ = reader_.copy_all_paths();
}

void cdi_helper::print_directory(const std::string &path) {
  reader_.read_directory(path, [](std::string name, const directory_entry &,
                                  const directory_entry_ex &) {
    if (!(name == "." || name == "..")) {
      std::cout << "    " << name << std::endl;
    }
    return true;
  });
}

fs::path cdi_helper::init_destination(std::string subdirectory_name /*= ""*/,
                                      bool create /*= true*/) {
  if (root_.empty()) {
    root_ = out_path_;

    const auto disc_label = parse::copy_disc_label(reader_.first_disc_label());
    if (!disc_label.empty()) {
      root_.append(disc_label);
    };

    fs::create_directories(root_);
  }

  fs::path subdirectory_path = root_;

  if (!subdirectory_name.empty() && subdirectory_name != ".") {
    subdirectory_path.append(subdirectory_name);
    if (create) {
      fs::create_directory(subdirectory_path);
    }
  }

  return subdirectory_path;
}

void cdi_helper::enum_directory(
    std::string path,
    std::function<void(const std::string &, const directory_entry &,
                       const directory_entry_ex &)>
        action) {
  std::unordered_map<std::string, directory_entry_2> files;
  reader_.read_directory(path, files);

  for (const auto &pair : files) {
    const std::string &name = pair.first;
    const directory_entry &file = pair.second.first;
    const directory_entry_ex &file_ex = pair.second.second;
    if (parse::is_directory(file_ex)) {
      continue;
    }

    action(name, file, file_ex);
  }
}

void cdi_helper::copy_mpeg_streams(const std::string &path,
                                   const directory_entry &file,
                                   const directory_entry_ex &file_ex,
                                   const fs::path &dest_directory) {
  std::unordered_map<std::string, std::ofstream> out_streams;
  bool media_found = false;

  reader_.scan_file(file, file_ex, [&](const sector_data &sector) {
    std::string stream_name;
    if (parse::is_mpeg_audio_sector(sector)) {
      stream_name =
          (boost::format("audio_channel_%d.mpeg") %
           static_cast<int>(parse::get_sector_header(sector).channel_num))
              .str();
    } else if (parse::is_mpeg_video_sector(sector)) {
      stream_name =
          (boost::format("video_channel_%d.mpeg") %
           static_cast<int>(parse::get_sector_header(sector).channel_num))
              .str();
    } else {
      // not interested in this sector, keep looking
      // debug_dump_sector_header(parse::get_sector_header(sector));
      return true;
    }

    if (!media_found) {
      fs::create_directories(dest_directory);
      media_found = true;
    }

    // open new output stream if needed
    if (out_streams.find(stream_name) == out_streams.end()) {
      fs::path stream_path = dest_directory;
      stream_path.append(stream_name);

      std::cerr << "    Copying " << stream_path << std::endl;

      // this opens new output stream
      out_streams.emplace(stream_name, stream_path.string());
    }

    std::ofstream &out_stream = out_streams.at(stream_name);

    // write chunk of media data to output
    if (parse::is_mode2_form1_sector(sector)) {
      out_stream.write(parse::get_mode2_form1_data<char>(sector),
                       mode2_form1_data_size);
    } else if (parse::is_mode2_form2_sector(sector)) {
      out_stream.write(parse::get_mode2_form2_data<char>(sector),
                       mode2_form2_data_size);
    } else {
      throw std::runtime_error("corrupted data");
    }
    return true;
  });
}
