//
//  dyuv.cpp
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 8/11/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#include "dyuv.h"

#include <png.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>

static bool write_png_file(std::vector<uint8_t> &rgb_data, const size_t width,
                           const size_t height, const std::string &path) {
  FILE *file = fopen(path.c_str(), "wb");
  if (!file) {
    return false;
  }

  auto png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    return false;
  }

  auto info_ptr = png_create_info_struct(png_ptr);
  if (!png_ptr) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return false;
  }

  png_set_IHDR(png_ptr, info_ptr, static_cast<png_uint_32>(width),
               static_cast<png_uint_32>(height), 8, PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  auto row_pointers =
      (uint8_t **)png_malloc(png_ptr, height * sizeof(uint8_t *));
  for (int y = 0; y < height; y++) {
    row_pointers[y] = &rgb_data[y * width * 3];
  }

  png_init_io(png_ptr, file);
  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  png_free(png_ptr, row_pointers);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(file);
  return true;
}

bool decode_dyuv(const std::vector<uint8_t> &dyuv_data,
                 const dyuv_options &options, std::vector<uint8_t> &rgb_data) {
  const size_t width = options.size.width;
  const size_t height = options.size.height;
  const size_t line_size = width;

  constexpr std::array<uint8_t, 16> coding_table = {
      {0, 1, 4, 9, 16, 27, 44, 79, 128, 177, 212, 229, 240, 247, 252, 255}};

  rgb_data.resize(width * height * 3);
  uint8_t *cur_out = &rgb_data[0];

  const uint8_t *current = &dyuv_data[0];
  const uint8_t *end = current + line_size * height;

  while (current < end) {
    uint8_t cur_y = options.seed.y;
    uint8_t cur_u = options.seed.u;
    uint8_t cur_v = options.seed.v;

    const uint8_t *next = current + line_size;

    while (current < next) {
      const uint8_t code_y0 = *current & 0x0f;
      const uint8_t code_u = *current++ >> 4;
      const uint8_t code_y1 = *current & 0x0f;
      const uint8_t code_v = *current++ >> 4;

      cur_y += coding_table[code_y0];
      cur_u += coding_table[code_u];
      cur_v += coding_table[code_v];

      const uint8_t y0 = cur_y;
      const uint8_t u0 = cur_u;
      const uint8_t v0 = cur_v;

      const int32_t b0 = (static_cast<int32_t>(y0) << 16) +
                         (static_cast<int32_t>(u0) - 128) * 113574 + 0x7fff;
      const int32_t r0 = (static_cast<int32_t>(y0) << 16) +
                         (static_cast<int32_t>(v0) - 128) * 89850 + 0x7fff;
      const int32_t g0 = static_cast<int32_t>(y0) * 111646 -
                         (r0 >> 16) * 33382 - (b0 >> 16) * 12728 + 0x7fff;

      *cur_out++ = std::clamp(r0, 0x000000, 0xffffff) >> 16;
      *cur_out++ = std::clamp(g0, 0x000000, 0xffffff) >> 16;
      *cur_out++ = std::clamp(b0, 0x000000, 0xffffff) >> 16;

      cur_y += coding_table[code_y1];

      const uint8_t y1 = cur_y;
      uint8_t u1, v1;
      if (options.interpolate && current < next) {
        const uint8_t next_code_u = *current >> 4;
        const uint8_t next_code_v = *(current + 1) >> 4;
        const uint8_t next_u = cur_u + coding_table[next_code_u];
        const uint8_t next_v = cur_v + coding_table[next_code_v];
        u1 = (static_cast<uint16_t>(cur_u) + next_u) >> 1;
        v1 = (static_cast<uint16_t>(cur_v) + next_v) >> 1;
      } else {
        u1 = cur_u;
        v1 = cur_v;
      }

      const int32_t b1 = (static_cast<int32_t>(y1) << 16) +
                         (static_cast<int32_t>(u1) - 128) * 113574 + 0x7fff;
      const int32_t r1 = (static_cast<int32_t>(y1) << 16) +
                         (static_cast<int32_t>(v1) - 128) * 89850 + 0x7fff;
      const int32_t g1 = static_cast<int32_t>(y1) * 111646 -
                         (r1 >> 16) * 33382 - (b1 >> 16) * 12728 + 0x7fff;

      *cur_out++ = std::clamp(r1, 0x000000, 0xffffff) >> 16;
      *cur_out++ = std::clamp(g1, 0x000000, 0xffffff) >> 16;
      *cur_out++ = std::clamp(b1, 0x000000, 0xffffff) >> 16;
    }

    current = next;
  }
  return true;
}

bool convert_dyuv_png(const std::vector<uint8_t> &dyuv_data,
                      const dyuv_options &options,
                      const std::string &destination) {
  std::vector<uint8_t> rgb_data;
  if (!decode_dyuv(dyuv_data, options, rgb_data)) {
    return false;
  }
  assert(rgb_data.size() == options.size.width * options.size.height * 3);
  if (!write_png_file(rgb_data, options.size.width, options.size.height,
                      destination)) {
    return false;
  }
  return true;
}
