#include <ares/ares.hpp>
#include "v30mz.hpp"

namespace ares {

enum : u32 { Byte = 1, Word = 2, Long = 4 };
#include "registers.cpp"
#include "memory.cpp"
#include "prefetch.cpp"
#include "modrm.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions-adjust.cpp"
#include "instructions-alu.cpp"
#include "instructions-exec.cpp"
#include "instructions-flag.cpp"
#include "instructions-group.cpp"
#include "instructions-misc.cpp"
#include "instructions-move.cpp"
#include "instructions-string.cpp"
#include "serialization.cpp"

auto V30MZ::power() -> void {
  static constexpr u16 Undefined = 0x0000;

  state.halt = 0;
  state.poll = 1;
  state.prefix = 0;

  opcode = 0;
  prefixes.flush();
  modrm.mod = 0;
  modrm.reg = 0;
  modrm.mem = 0;
  modrm.segment = 0;
  modrm.address = 0;

  AW  = Undefined;
  CW  = Undefined;
  DW  = Undefined;
  BW  = Undefined;
  SP  = Undefined;
  BP  = Undefined;
  IX  = Undefined;
  IY  = Undefined;
  DS1 = 0x0000;
  PS  = 0xffff;
  SS  = 0x0000;
  DS0 = 0x0000;
  PC  = 0x0000;
  PSW = 0x8000;
  flush();
}

#undef bits
#include "disassembler.cpp"

}
