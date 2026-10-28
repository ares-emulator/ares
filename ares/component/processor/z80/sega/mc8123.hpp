#pragma once

#include <component/processor/z80/sega/base.hpp>

namespace ares {

struct MC8123 : SegaZ80 {
  u8 keyData[0x2000] = {};
  u16 encryptedSize = 0xc000;

  auto decrypt(n16 address, n8 source, bool opcode) const -> n8 override;

private:
  auto reorder(n8 source, const u8 (&bits)[8]) const -> n8;
  auto permute(n8 source, n3 type, n2 permutation) const -> n8;
  auto type0(n8 value, n4 parameter, n2 permutation) const -> n8;
  auto type1a(n8 value, n4 parameter, n2 permutation) const -> n8;
  auto type1b(n8 value, n4 parameter, n2 permutation) const -> n8;
  auto type2a(n8 value, n4 parameter, n2 permutation) const -> n8;
  auto type2b(n8 value, n4 parameter, n2 permutation) const -> n8;
  auto type3a(n8 value, n4 parameter, n2 permutation) const -> n8;
  auto type3b(n8 value, n4 parameter, n2 permutation) const -> n8;
  auto transform(n8 value, n8 key, bool opcode) const -> n8;
};

}
