#include <ares/ares.hpp>
#include "tlcs900h.hpp"

namespace ares {

#include "registers.cpp"
#include "control-registers.cpp"
#include "prefetch.cpp"
#include "memory.cpp"
#include "conditions.cpp"
#include "algorithms.cpp"
#include "dma.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto TLCS900H::interrupt(n8 vector) -> void {
  prefetch(34);
  push(PC);
  push(SR);
  store(PC, load(Memory<n32>{0xffff00 | vector}));
  store(INTNEST, load(INTNEST) + 1);
}

auto TLCS900H::power() -> void {
  r = {};
  store(XSP, 0x100);

  CF = CA = 0;
  NF = NA = 0;
  VF = VA = 0;
  HF = HA = 0;
  ZF = ZA = 0;
  SF = SA = 0;
  RFP = 0;
  IFF = 7;

  OP = 0;
  HALT = 0;
  PIC = 0;
  PIQ.flush();
  MAR = 0;
  MDR = 0;
}

}
