#include <ares/ares.hpp>
#include "sh2.hpp"

namespace ares {

#define SP R[15]

#include "sh7604/sh7604.cpp"
#include "exceptions.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "recompiler.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto SH2::power(bool reset) -> void {
  for(auto& r : R) r = undefined;
  SR.T = undefined;
  SR.S = undefined;
  SR.I = 0b1111;
  SR.Q = undefined;
  SR.M = undefined;
  GBR = undefined;
  VBR = 0;
  MACH = undefined;
  MACL = undefined;
  PR = undefined;
  SP = 0;
  PC = 0;
  PPC = 0;
  PPM = Branch::Step;
  ID = 0;
  exceptions = !reset ? ResetCold : ResetWarm;

  cache = {*this};
  intc = {*this};
  dmac = {*this};
  sci = {*this};
  wdt = {};
  ubc = {};
  frt = {*this};
  bsc = {};
  sbycr = {};
  divu = {};

  cache.power();

  if constexpr(Accuracy::Recompiler) {
    recompiler.allocator.resize(512_MiB, bump_allocator::executable | bump_allocator::zero_fill);
    recompiler.reset();
    recompiler.clock = 0;
  }
}

}
