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

struct dyuv_options {
  size_t width = 384;
  size_t height = 280;
  uint8_t init_y = 16;
  uint8_t init_u = 128;
  uint8_t init_v = 128;
  bool interpolate = true;
};

bool decode_dyuv(const std::vector<uint8_t> &dyuv_data,
                 const dyuv_options &options, std::vector<uint8_t> &rgb_data);

bool convert_dyuv_png(const std::vector<uint8_t> &dyuv_data,
                      const dyuv_options &options,
                      const std::string &destination);
