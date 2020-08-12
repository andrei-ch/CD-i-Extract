//
//  actions.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include <string>

#include "dyuv.h"

struct action_options {
  dyuv_options dyuv;
};

int print_filesystem(std::string input_path, std::string output_path,
                     const action_options &opts);
int copy_filesystem(std::string input_path, std::string output_path,
                    const action_options &opts);
int copy_mpeg_streams(std::string input_path, std::string output_path,
                      const action_options &opts);
int copy_dyuv_images(std::string input_path, std::string output_path,
                     const action_options &opts);
int copy_all(std::string input_path, std::string output_path,
             const action_options &opts);
