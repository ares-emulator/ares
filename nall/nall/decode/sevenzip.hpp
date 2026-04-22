#pragma once

#include <cassert>
#include <nall/maybe.hpp>
#include <nall/string.hpp>
#include <utility>
#include <vector>

namespace nall::Decode {

struct SevenZip {
  struct File {
    string name;
    u64 size;
    u32 index;
    bool directory;
    time_t timestamp;
  };

  ~SevenZip();

  auto findFile(const string& filename) const -> const maybe<File>;
  auto open(const string& filename) -> bool;
  auto extract(const File& file) -> std::vector<u8>;
  auto close() -> void;

  std::vector<File> file;

private:
  struct State;
  State* state = nullptr;
};

}
