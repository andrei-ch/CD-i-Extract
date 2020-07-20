//
//  helper.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include "cdi_lib/parse.h"

#include <boost/filesystem.hpp>

class cdi_helper {
public:
  cdi_helper(std::string in_path, std::string out_path = "")
      : reader_(in_path), out_path_(out_path) {}

  boost::filesystem::path init_destination(std::string subdirectory_name = "",
                                           bool create = true);

  void read_disc_paths();
  const std::vector<std::string> &disc_paths() const { return paths_; }

  void print_directory(const std::string &disc_path);

  void enum_directory(
      std::string disc_path,
      std::function<void(const std::string &, const cd_i::directory_entry &,
                         const cd_i::directory_entry_ex &)>);

  void copy_mpeg_streams(const std::string &disc_path,
                         const cd_i::directory_entry &file,
                         const cd_i::directory_entry_ex &file_ex,
                         const boost::filesystem::path &dest_directory);

  cd_i::disc_structure_reader &reader() { return reader_; }

private:
  std::string out_path_;
  std::vector<std::string> paths_;
  cd_i::disc_structure_reader reader_;
  boost::filesystem::path root_;
};
