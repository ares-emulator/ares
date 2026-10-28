#pragma once

#include <component/processor/z80/z80.hpp>

namespace ares {

struct SegaZ80 {
  virtual ~SegaZ80() = default;
  virtual auto decrypt(n16 address, n8 source, bool opcode) const -> n8 = 0;
};

}
