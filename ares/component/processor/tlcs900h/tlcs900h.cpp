#include <ares/ares.hpp>
#include "tlcs900h.hpp"

namespace ares {

#define CF r.c
#define NF r.n
#define VF r.v
#define PF r.v
#define HF r.h
#define ZF r.z
#define SF r.s

#define RFP r.rfp
#define IFF r.iff

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
  wait(19);
  push(PC);
  push(SR);
  store(PC, load(Memory<n32>{0xffff00 | vector}));
  store(INTNEST, load(INTNEST) + 1);
}

auto TLCS900H::power() -> void {
  r = {};
  r.xsp.l.l0 = 0x100;
  MAR = 0;
  MDR = 0;
  invalidate();
}

}
