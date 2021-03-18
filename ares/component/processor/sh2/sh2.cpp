#include <ares/ares.hpp>
#include "sh2.hpp"

namespace ares {

#define SP R[15]

#include "memory.cpp"
#include "sh7604.cpp"
#include "sh7604-intc.cpp"
#include "sh7604-dmac.cpp"
#include "sh7604-frt.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto SH2::power() -> void {
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

  for(auto& byte : cache) byte = 0;

  intc = {*this};
  dmac = {*this};
  sci = {};
  wdt = {};
  ubc = {};
  frt = {*this};
  bsc = {};
  ccr = {};
  sbycr = {};
  divu = {};
}

}
