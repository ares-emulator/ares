#pragma once

//Fujitsu MBM29F016A: 16Mbit (2MiB) 8-bit parallel flash

#include <vector>

namespace ares {

struct MBM29F016A {
  auto power() -> void { state = 0; }
  auto read(u32 address) -> u8;
  auto write(u32 address, u8 value) -> void;
  auto serialize(serializer&) -> void;

  std::vector<u8> data;
  u32 state = 0;
};

}
