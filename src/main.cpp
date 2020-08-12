//
//  main.cpp
//  CD-i Extract
//
//  Created by Andrei Chtcherbatchenko on 7/19/20.
//  Copyright © 2020 Andrei Chtcherbatchenko. All rights reserved.
//

#include "actions.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <iostream>

namespace {

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using command_handler =
    std::function<int(std::string, std::string, const action_options &options)>;

struct command_handler_t {
  command_handler value;
};

struct command_description {
  std::string name;
  std::string description;
  command_handler handler;
};

const std::array<command_description, 5> commands = {{
    {"print,p", "Print all files and directories in CD-i track image",
     &print_filesystem},
    {"extract-files,x",
     "Copy files and directories from CD-i track image. (Note: MPEG streams "
     "are not files.)",
     &copy_filesystem},
    {"extract-mpegs,m", "Copy real-time MPEG streams from CD-i track image",
     &copy_mpeg_streams},
    {"extract-dyuv,d", "Copy DYUV images from CD-i track image",
     &copy_dyuv_images},
    {"extract-all,a", "Copy all supported formats from CD-i track image",
     &copy_all},
}};

command_handler_t action;
std::string input_path;
std::string output_path;
action_options options;

struct dyuv_size_t {
  dyuv_size value;
};

struct dyuv_seed_t {
  dyuv_seed value;
};

constexpr std::array<dyuv_size, 3> supported_dyuv_sizes = {
    {{384, 280}, {384, 240}, {360, 240}}};

std::string dyuv_size_str(const dyuv_size &size) {
  return (boost::format("%lu:%lu") % size.width % size.height).str();
}

std::string supported_dyuv_sizes_str() {
  std::vector<std::string> sizes;
  std::transform(supported_dyuv_sizes.begin(), supported_dyuv_sizes.end(),
                 std::back_inserter(sizes),
                 [](const dyuv_size &size) { return dyuv_size_str(size); });
  return boost::algorithm::join(sizes, ", ");
}

std::string dyuv_seed_str(const dyuv_seed &seed) {
  return (boost::format("%u:%u:%u") % static_cast<unsigned int>(seed.y) %
          static_cast<unsigned int>(seed.u) % static_cast<unsigned int>(seed.v))
      .str();
}

void validate(boost::any &v, const std::vector<std::string> &values,
              dyuv_size_t *, int) {
  const auto s = po::validators::get_single_string(values);
  for (const auto &size : supported_dyuv_sizes) {
    if (s == dyuv_size_str(size)) {
      v = boost::any(dyuv_size_t{.value = size});
      return;
    }
  }
  throw po::validation_error(po::validation_error::invalid_option_value);
}

void validate(boost::any &v, const std::vector<std::string> &values,
              dyuv_seed_t *, int) {
  const auto s = po::validators::get_single_string(values);
  std::vector<std::string> components;
  boost::split(components, s, boost::is_any_of(":"));
  if (components.size() != 3) {
    throw po::validation_error(po::validation_error::invalid_option_value);
  }
  const auto sy = std::stoul(components[0]);
  const auto su = std::stoul(components[1]);
  const auto sv = std::stoul(components[2]);
  if (sy > 255 || su > 255 || sv > 255) {
    throw po::validation_error(po::validation_error::invalid_option_value);
  }
  v = boost::any(
      dyuv_seed_t{.value = dyuv_seed{.y = static_cast<uint8_t>(sy),
                                     .u = static_cast<uint8_t>(su),
                                     .v = static_cast<uint8_t>(sv)}});
}

void validate(boost::any &v, const std::vector<std::string> &values,
              command_handler_t *, int) {
  const auto s = po::validators::get_single_string(values);
  for (const auto &c : commands) {
    std::vector<std::string> spellings;
    boost::split(spellings, c.name, boost::is_any_of(","));
    if (std::find(spellings.begin(), spellings.end(), s) != spellings.end()) {
      v = boost::any(command_handler_t{.value = c.handler});
      return;
    }
  }
  throw po::validation_error(po::validation_error::invalid_option_value);
}

void print_commands() {
  std::cerr << "Commands:" << std::endl;

  for (const auto &c : commands) {
    std::vector<std::string> spellings;
    boost::split(spellings, c.name, boost::is_any_of(","));

    std::string syntax;
    if (spellings.size() == 1) {
      syntax = (boost::format("  %s") % spellings[0]).str();
    } else {
      syntax =
          (boost::format("  %s [ %s ]") % spellings[0] % spellings[1]).str();
    }

    std::cerr.width(24);
    std::cerr.fill(' ');
    std::cerr << std::left << syntax << c.description << std::endl;
  }
}

bool parse_options(int argc, const char *argv[]) {
  bool usage;
  std::string command;
  dyuv_size_t size;
  dyuv_seed_t seed;
  bool no_interpolation;

  const std::string dyuv_size_description =
      std::string("DYUV dimensions (supported: ") + supported_dyuv_sizes_str() +
      ", default: " + dyuv_size_str(supported_dyuv_sizes.front()) + ")";
  const std::string dyuv_seed_description =
      std::string("DYUV initial vector (default: ") +
      dyuv_seed_str(seed.value) + ")";

  po::options_description global_options("Options");
  global_options.add_options()("help,h", po::bool_switch(&usage),
                               "produce this help message")(
      "dyuv-size,", po::value<dyuv_size_t>(&size),
      dyuv_size_description.c_str())("dyuv-init,",
                                     po::value<dyuv_seed_t>(&seed),
                                     dyuv_seed_description.c_str())(
      "dyuv-no-interpolation,", po::bool_switch(&no_interpolation),
      "disable DYUV interpolation");

  po::options_description hidden_options;
  hidden_options.add_options()("command", po::value(&action)->required(),
                               "command to execute")(
      "input-path", po::value(&input_path)->required(), "path to input file")(
      "output-path", po::value(&output_path), "path to output file");

  po::options_description all_options;
  all_options.add(global_options).add(hidden_options);

  po::positional_options_description positional_options;
  positional_options.add("command", 1);
  positional_options.add("input-path", 1);
  positional_options.add("output-path", 1);

  try {
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(all_options)
                  .positional(positional_options)
                  .run(),
              vm);
    po::notify(vm);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    usage = true;
  }

  if (usage) {
    fs::path path(argv[0]);
    std::cerr << "Usage: " << path.filename().string()
              << " <command> [options] <input_path> [<output_path>]"
              << std::endl
              << std::endl;
    print_commands();
    std::cerr << std::endl;
    std::cerr << global_options << std::endl;
    return false;
  }

  options.dyuv.size = size.value;
  options.dyuv.seed = seed.value;
  options.dyuv.interpolate = !no_interpolation;

  if (output_path.empty()) {
    boost::filesystem::path path(input_path);
    output_path = path.parent_path().string();
  }
  return true;
}

} // namespace

int main(int argc, const char *argv[]) {
  if (!parse_options(argc, argv)) {
    return 1;
  }

  return action.value(input_path, output_path, options);
}
