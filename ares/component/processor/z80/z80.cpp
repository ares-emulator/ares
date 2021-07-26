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
  IM = 1;
}

auto Z80::irq(bool maskable, n16 pc, n8 extbus) -> bool {
  //do not execute maskable interrupts if disabled or immediately after EI instruction
  if(maskable && (!IFF1 || EI)) return false;
  R.bit(0,6)++;

  push(PC);

  switch(maskable ? IM : (n2)1) {

  case 0: {
    //external data bus ($ff = RST $38)
    WZ = extbus;
    wait(extbus | 0x38 == 0xff ? 6 : 7);
    break;
  }

  case 1: {
    //constant address
    WZ = pc;
    wait(maskable ? 7 : 5);
    break;
  }

  case 2: {
    //vector table with external data bus
    n16 address = I << 8 | extbus;
    WZL = read(address + 0);
    WZH = read(address + 1);
    wait(7);
    break;
  }

  }

  PC = WZ;
  IFF1 = 0;
  if(maskable) IFF2 = 0;
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
