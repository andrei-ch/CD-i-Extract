//
//  dyuv.h
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 8/11/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

struct dyuv_size {
  size_t width = 384;
  size_t height = 280;
};

struct dyuv_seed {
  uint8_t y = 16;
  uint8_t u = 128;
  uint8_t v = 128;
};

struct dyuv_options {
  dyuv_size size;
  dyuv_seed seed;
  bool interpolate = true;
};

bool decode_dyuv(const std::vector<uint8_t> &dyuv_data,
                 const dyuv_options &options, std::vector<uint8_t> &rgb_data);

bool convert_dyuv_png(const std::vector<uint8_t> &dyuv_data,
                      const dyuv_options &options,
                      const std::string &destination);
