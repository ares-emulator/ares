#include <ares/ares.hpp>
#include "z80.hpp"

namespace ares {

#include "disassembler.cpp"
#include "registers.hpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "algorithms.cpp"
#include "instructions.cpp"
#include "serialization.cpp"

auto Z80::power(MOSFET mosfet) -> void {
  this->mosfet = mosfet;

  prefix = Prefix::hl;
  af.word = 0xffff; af_.word = 0x0000;
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
  IFF1 = 0;
  IFF2 = 0;
  IM = 0;
}

auto Z80::reset() -> void {
  prefix = Prefix::hl;
  ir.word = 0x0000;
  WZ = PC = 0x0000;
  EI = 0;
  HALT = 0;
  IFF1 = 0;
  IFF2 = 0;
  IM = 0;
}

auto Z80::irq(n8 extbus) -> bool {
  //do not execute maskable interrupts if disabled or immediately after EI instruction
  if(!IFF1 || EI) return false;
  R.bit(0,6)++;

  switch(IM) {
  case 1: extbus = 0xff;
  case 0: {
    WZ = extbus;
    wait(6);
    instruction(WZ);
    break;
  }

  case 2: {
    //vector table with external data bus
    n16 address = I << 8 | extbus;
    WZL = read(address + 0);
    WZH = read(address + 1);
    wait(7);
    push(PC);
    PC = WZ;
    break;
  }

  }

  IFF1 = 0;
  IFF2 = 0;
  HALT = 0;
  if(P) PF = 0;
  P = 0;
  Q = 0;

  return true;
}

auto Z80::nmi() -> bool {
  R.bit(0,6)++;

  push(PC);
  WZ = 0x66;
  wait(5);
  PC = WZ;

  IFF1 = 0;
  HALT = 0;
  if(P) PF = 0;
  P = 0;
  Q = 0;

  return true;
}

auto Z80::parity(n8 value) const -> bool {
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  return !(value & 1);
}

}
