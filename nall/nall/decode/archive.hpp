#pragma once

#include <cassert>
#include <ctime>
#include <span>
#include <string>
#include <vector>
#include <nall/maybe.hpp>
#include <nall/string.hpp>

namespace nall::Decode {

// Minimal read-only interface shared by archive container decoders.
//
// Archive entries are value objects so callers can retain an entry while the
// archive remains open. Backends may use the format-specific fields internally;
// callers should prefer the Archive operations rather than interpreting them.
struct Archive {
  struct File {
    string name;
    const u8* data = nullptr;
    u64 size = 0;
    u64 csize = 0;
    u32 cmode = 0;
    u32 crc32 = 0;
    time_t timestamp = 0;
  };

  virtual ~Archive() = default;

  // Archive member names are a virtual namespace. They are never used as
  // extraction paths, and all callers resolve CUE references through these
  // helpers before looking up a member.
  static auto normalizeMemberName(const string& name) -> maybe<string> {
    std::string input{name.data(), name.size()};
    for(auto& character : input) if(character == '\\') character = '/';

    if(input.empty() || input.front() == '/') return nothing;
    if(input.size() >= 2 && input[1] == ':') return nothing;

    std::vector<std::string> components;
    size_t offset = 0;
    while(offset <= input.size()) {
      auto separator = input.find('/', offset);
      auto component = input.substr(offset, separator - offset);
      if(!component.empty() && component != ".") {
        if(component == "..") return nothing;
        components.push_back(std::move(component));
      }
      if(separator == std::string::npos) break;
      offset = separator + 1;
    }

    if(components.empty()) return nothing;
    std::string output;
    for(auto& component : components) {
      if(!output.empty()) output += '/';
      output += component;
    }
    return string{output.c_str()};
  }

  static auto resolveMemberName(const string& folder, const string& reference) -> maybe<string> {
    std::string base{folder.data(), folder.size()};
    std::string relative{reference.data(), reference.size()};
    for(auto& character : base) if(character == '\\') character = '/';
    for(auto& character : relative) if(character == '\\') character = '/';

    if(relative.empty() || relative.front() == '/') return nothing;
    if(relative.size() >= 2 && relative[1] == ':') return nothing;

    std::vector<std::string> components;
    auto append = [&](const std::string& path, bool allowParent) -> bool {
      size_t offset = 0;
      while(offset <= path.size()) {
        auto separator = path.find('/', offset);
        auto component = path.substr(offset, separator - offset);
        if(!component.empty() && component != ".") {
          if(component == "..") {
            if(!allowParent || components.empty()) return false;
            components.pop_back();
          } else {
            components.push_back(std::move(component));
          }
        }
        if(separator == std::string::npos) break;
        offset = separator + 1;
      }
      return true;
    };

    if(!append(base, false) || !append(relative, true) || components.empty()) return nothing;

    std::string output;
    for(auto& component : components) {
      if(!output.empty()) output += '/';
      output += component;
    }
    return string{output.c_str()};
  }

  virtual auto findFile(const string& filename) const -> const maybe<File> = 0;
  virtual auto files() const -> std::vector<File> = 0;
  virtual auto open(const string& filename) -> bool = 0;
  virtual auto extract(const File& file) const -> std::vector<u8> = 0;
  virtual auto isDataUncompressed(const File& file) const -> bool = 0;
  virtual auto dataViewIfUncompressed(const File& file) const -> std::span<const u8> = 0;
  virtual auto error() const -> string = 0;
  virtual auto close() -> void = 0;
};

}
