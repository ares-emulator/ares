#pragma once

#include <nall/string.hpp>
#include <nall/directory.hpp>
#include <nall/file.hpp>
#include <nall/location.hpp>
#include <nall/path.hpp>
#include <vector>

namespace nall {

struct Arguments {
  Arguments(int argc, char** argv);
  Arguments(std::vector<string> arguments);

  explicit operator bool() const { return !arguments.empty(); }
  auto size() const -> u32 { return arguments.size(); }

  auto operator[](u32 index) -> string& { return arguments[index]; }
  auto operator[](u32 index) const -> const string& { return arguments[index]; }

  auto programPath() const -> string;
  auto programName() const -> string;
  auto programLocation() const -> string;

  auto find(string_view name) const -> bool;
  auto find(string_view name, bool& argument) const -> bool;
  auto find(string_view name, string& argument) const -> bool;

  auto begin() const { return arguments.begin(); }
  auto end() const { return arguments.end(); }

  auto rbegin() const { return arguments.rbegin(); }
  auto rend() const { return arguments.rend(); }

  auto take() -> string;
  auto take(string_view name) -> bool;
  auto take(string_view name, bool& argument) -> bool;
  auto take(string_view name, string& argument) -> bool;

  auto begin() { return arguments.begin(); }
  auto end() { return arguments.end(); }

  auto rbegin() { return arguments.rbegin(); }
  auto rend() { return arguments.rend(); }

private:
  auto construct() -> void;

  string programArgument;
  std::vector<string> arguments;
};

inline auto Arguments::construct() -> void {
  if(arguments.empty()) return;

  //extract and pre-process program argument
  programArgument = arguments.front();
  arguments.erase(arguments.begin());
  programArgument = {Path::real(programArgument), Location::file(programArgument)};

  //normalize path and file arguments
  for(auto& argument : arguments) {
    argument = directory::resolveSymLink(argument);
    if(directory::exists(argument)) {
      argument.transform("\\", "/").trimRight("/").append("/");
      argument = Path::real(argument);
    }
    else if(file::exists(argument)) {
      argument.transform("\\", "/").trimRight("/");
      argument = {Path::real(argument), Location::file(argument)};
    }
  }
}

inline Arguments::Arguments(int argc, char** argv) {
  #if defined(PLATFORM_WINDOWS)
  utf8_arguments(argc, argv);
  #endif
  for(u32 index : range(argc)) arguments.push_back(argv[index]);
  construct();
}

inline Arguments::Arguments(std::vector<string> arguments) {
  this->arguments = std::move(arguments);
  construct();
}

inline auto Arguments::programPath() const -> string {
  return Location::path(programArgument);
}

inline auto Arguments::programName() const -> string {
  return Location::file(programArgument);
}

inline auto Arguments::programLocation() const -> string {
  return programArgument;
}

inline auto Arguments::find(string_view name) const -> bool {
  for(u32 index : range(arguments.size())) {
    if(arguments[index].match(name)) {
      return true;
    }
  }
  return false;
}

inline auto Arguments::find(string_view name, bool& argument) const -> bool {
  for(u32 index : range(arguments.size())) {
    if(arguments[index].match(name) && arguments.size() >= index
    && (arguments[index + 1] == "true" || arguments[index + 1] == "false")) {
      argument = arguments[index + 1] == "true";
      return true;
    }
  }
  return false;
}

inline auto Arguments::find(string_view name, string& argument) const -> bool {
  for(u32 index : range(arguments.size())) {
    if(arguments[index].match(name) && arguments.size() >= index) {
      argument = arguments[index + 1];
      return true;
    }
  }
  return false;
}

//

inline auto Arguments::take() -> string {
  if(arguments.empty()) return {};
  auto out = arguments.front();
  arguments.erase(arguments.begin());
  return out;
}

inline auto Arguments::take(string_view name) -> bool {
  for(u32 index : range(arguments.size())) {
    if(arguments[index].match(name)) {
      arguments.erase(arguments.begin() + index);
      return true;
    }
  }
  return false;
}

inline auto Arguments::take(string_view name, bool& argument) -> bool {
  for(u32 index : range(arguments.size())) {
    if(arguments[index].match(name) && arguments.size() > index + 1
    && (arguments[index + 1] == "true" || arguments[index + 1] == "false")) {
      arguments.erase(arguments.begin() + index);
      argument = arguments[index] == "true";
      arguments.erase(arguments.begin() + index);
      return true;
    }
  }
  return false;
}

inline auto Arguments::take(string_view name, string& argument) -> bool {
  for(u32 index : range(arguments.size())) {
    if(arguments[index].match(name) && arguments.size() > index + 1) {
      arguments.erase(arguments.begin() + index);
      argument = arguments[index];
      arguments.erase(arguments.begin() + index);
      return true;
    }
  }
  return false;
}

}
