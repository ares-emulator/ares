#pragma once

//Sharp LH28F400: 4Mbit (512KiB) 16-bit parallel flash

#include <vector>

namespace ares {

struct LH28F400 {
  auto power() -> void { state = 0; }
  auto read(u32 word) -> u32;
  auto write(u32 word, u32 value) -> void;
  auto serialize(serializer&) -> void;

  std::vector<u8> data;
  u32 state = 0;
};

}
