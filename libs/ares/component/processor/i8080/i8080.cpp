#include <ares/ares.hpp>
#include "i8080.hpp"

namespace ares {

#include "disassembler.cpp"
#include "registers.hpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "algorithms.cpp"
#include "instructions.cpp"
#include "serialization.cpp"

auto I8080::power() -> void {
  prefix = Prefix::hl;
  af.word = 0xffd7; af_.word = 0x0000;
  bc.word = 0x0000; bc_.word = 0x0000;
  de.word = 0x0000; de_.word = 0x0000;
  hl.word = 0x0000; hl_.word = 0x0000;
  ix.word = 0x0000;
  iy.word = 0x0000;
  ir.word = 0x0000;
  wz.word = 0x0000;
  SP = 0xffff;
  PC = 0x0000;
  EI = 0;
  P = 0;
  Q = 0;
  HALT = 0;
  INTE = 0;
}

auto I8080::reset() -> void {
  prefix = Prefix::hl;
  ir.word = 0x0000;
  WZ = PC = 0x0000;
  EI = 0;
  HALT = 0;
  INTE = 0;
}

auto I8080::irq(n8 extbus) -> bool {
  //do not execute maskable interrupts if disabled or immediately after EI instruction
  if(!INTE || EI) return false;

  WZ = extbus;
  wait(6);
  instruction(WZ);

  INTE = 0;
  HALT = 0;
  if(P) PF = 0;
  P = 0;
  Q = 0;

  return true;
}

auto I8080::nmi() -> bool {
  push(PC);
  WZ = 0x66;
  wait(5);
  PC = WZ;

  INTE = 0;
  HALT = 0;
  if(P) PF = 0;
  P = 0;
  Q = 0;

  return true;
}

auto I8080::parity(n8 value) const -> bool {
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  return !(value & 1);
}

}
