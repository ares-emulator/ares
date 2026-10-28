#pragma once

#include <component/processor/z80/sega/base.hpp>

namespace ares {

struct S315_5114 : SegaZ80 {
  auto decrypt(n16 address, n8 source, bool opcode) const -> n8 override;
};

}
