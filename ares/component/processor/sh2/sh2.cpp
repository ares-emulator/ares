#define XXH_INLINE_ALL
#include <xxhash.h>

#include <ares/ares.hpp>
#include "sh2.hpp"

namespace ares {

#define SP   R[15]
#define R    regs.R
#define PC   regs.PC
#define PR   regs.PR
#define GBR  regs.GBR
#define VBR  regs.VBR
#define MAC  regs.MAC
#define MACL regs.MACL
#define MACH regs.MACH
#define CCR  regs.CCR
#define SR   regs.SR
#define PPC  regs.PPC
#define PPM  regs.PPM
#define ET   regs.ET
#define ID   regs.ID

#include "sh7604/sh7604.cpp"
#include "exceptions.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto SH2::power(bool reset) -> void {
  for(auto& r : R) r = undefined;
  PC = 0;
  PR = undefined;
  GBR = undefined;
  VBR = 0;
  MACH = undefined;
  MACL = undefined;
  CCR = 0;
  SR.T = undefined;
  SR.S = undefined;
  SR.I = 0b1111;
  SR.Q = undefined;
  SR.M = undefined;
  PPC = 0;
  PPM = Branch::Step;
  ET = 0;
  ID = 0;
  exceptions = !reset ? ResetCold : ResetWarm;
  cyclesUntilRecompilerExit = 0;

  cache = {*this};
  intc = {*this};
  dmac = {*this};
  sci = {*this, reset ? sci.link : maybe<SH2&>()};
  wdt = {};
  ubc = {};
  frt = {*this};
  bsc = {};
  sbycr = {};
  divu = {};

  cache.power();

  if constexpr(Accuracy::Recompiler) {
    if(!reset) {
      auto buffer = ares::Memory::FixedAllocator::get().tryAcquire(32_MiB);
      recompiler.allocator.resize(32_MiB, bump_allocator::executable, buffer);
    }
    recompiler.reset();
  }
}

#undef SP
#undef R
#undef PC
#undef PR
#undef GBR
#undef VBR
#undef MAC
#undef MACL
#undef MACH
#undef CCR
#undef SR
#undef PPC
#undef PPM
#undef ET
#undef ID

//#include "cached.cpp"
#include "recompiler.cpp"

}
