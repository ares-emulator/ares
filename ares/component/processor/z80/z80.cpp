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
  r = {};
  AF = 0xffff;
  SP = 0xffff;
  IFF1 = 0;
  IFF2 = 0;
  IM = 1;
}

auto Z80::irq(bool maskable, uint16 pc, uint8 extbus) -> bool {
  if(maskable && !IFF1) return false;
  R.bit(0,6)++;

  push(PC);

  switch(maskable ? IM : (uint2)1) {

  case 0: {
    //external data bus ($ff = RST $38)
    WZ = extbus;
    break;
  }

  case 1: {
    //constant address
    WZ = pc;
    break;
  }

  case 2: {
    //vector table with external data bus
    uint16 addr = I << 8 | extbus;
    WZL = read(addr + 0);
    WZH = read(addr + 1);
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

auto Z80::parity(uint8 value) const -> bool {
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  return !(value & 1);
}

}
