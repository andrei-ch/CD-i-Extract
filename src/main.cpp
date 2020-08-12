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

using command_handler = std::function<int(std::string, std::string)>;

struct command_handler_t {
  command_handler value;
};

struct command_description {
  std::string name;
  std::string description;
  command_handler handler;
};

const std::array<command_description, 4> commands = {{
    {"print,p", "Print all files and directories in CD-i track image",
     &print_filesystem},
    {"extract-files,x",
     "Copy files and directories from CD-i track image. (Note: MPEG streams "
     "are not files.)",
     &copy_filesystem},
    {"extract-mpegs,m", "Copy real-time MPEG streams from CD-i track image",
     &copy_mpeg_streams},
    {"extract-all,a", "Copy everything from CD-i track image (same as -x -m)",
     &copy_all},
}};

command_handler_t action;
std::string input_path;
std::string output_path;

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

  po::options_description global_options("Options");
  global_options.add_options()("help,h", po::bool_switch(&usage),
                               "produce this help message");

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

  return action.value(input_path, output_path);
}
