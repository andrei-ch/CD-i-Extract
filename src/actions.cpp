//
//  actions.cpp
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#include "actions.h"

#include "dyuv.h"
#include "helper.h"

using namespace cd_i;

namespace fs = boost::filesystem;

int print_filesystem(std::string input_path, std::string /*output_path*/) {
  try {
    cdi_helper worker(input_path);
    worker.read_disc_paths();
    for (const auto &path : worker.disc_paths()) {
      std::cout << "/" << path << std::endl;

      worker.print_directory(path);

      std::cout << std::endl;
    }
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}

int copy_filesystem(std::string input_path, std::string output_path) {
  try {
    cdi_helper worker(input_path, output_path);
    worker.read_disc_paths();
    worker.init_destination();

    for (const auto &path : worker.disc_paths()) {
      std::cout << "/" << path << std::endl;

      fs::path subdirectory = worker.init_destination(path);

      worker.enum_directory(path, [&](const std::string &name,
                                      const directory_entry &file,
                                      const directory_entry_ex &file_ex) {
        fs::path destination = subdirectory;
        destination.append(name);

        std::cerr << "    Copying " << destination.string() << std::endl;
        worker.reader().copy_file(file, file_ex, destination.string());
      });

      std::cout << std::endl;
    }
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}

int copy_mpeg_streams(std::string input_path, std::string output_path) {
  try {
    cdi_helper worker(input_path, output_path);
    worker.read_disc_paths();
    worker.init_destination();

    for (const auto &path : worker.disc_paths()) {
      worker.enum_directory(path, [&](const std::string &name,
                                      const directory_entry &file,
                                      const directory_entry_ex &file_ex) {
        // Add .MEDIA suffix to stream directory name to prevent overwriting an
        // actual file
        const auto destination =
            worker.init_destination(path + "/" + name + ".MEDIA", false);

        worker.copy_mpeg_streams(path, file, file_ex, destination);
      });
    }
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}

int copy_dyuv_images(std::string input_path, std::string output_path) {
  try {
    cdi_helper worker(input_path, output_path);
    worker.read_disc_paths();
    worker.init_destination();

    for (const auto &path : worker.disc_paths()) {
      worker.enum_directory(path, [&](const std::string &name,
                                      const directory_entry &file,
                                      const directory_entry_ex &file_ex) {
        // Add .MEDIA suffix to stream directory name to prevent overwriting an
        // actual file
        const auto destination =
            worker.init_destination(path + "/" + name + ".MEDIA", false);

        worker.copy_dyuv_images(path, file, file_ex, dyuv_options{},
                                destination);
      });
    }
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}

int copy_all(std::string input_path, std::string output_path) {
  copy_filesystem(input_path, output_path);
  copy_mpeg_streams(input_path, output_path);
  copy_dyuv_images(input_path, output_path);
  return 0;
}
