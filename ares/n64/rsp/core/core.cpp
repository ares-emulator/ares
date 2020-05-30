#include "registers.hpp"
#include "vu-registers.cpp"
#include "decoder.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "vu-instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto RSP::instruction() -> void {
  pipeline.address = PC;
  pipeline.instruction = imem.readWord(pipeline.address);

  debugger.instruction();
//instructionDEBUG();
  instructionEXECUTE();
  GPR[0].u32 = 0;

  switch(branch.state) {
  case Branch::Step: PC += 4; break;
  case Branch::DelaySlot: PC = branch.pc; branch.reset(); break;
  case Branch::Take: PC += 4; branch.delaySlot(); break;
  }
}

auto RSP::instructionDEBUG() -> void {
  static uint counter = 0;
//if(++counter > 100) return;
  print(
    disassembler.hint(hex(pipeline.address, 3L)), "  ",
  //disassembler.hint(hex(pipeline.instruction, 8L)), "  ",
    disassembler.disassemble(pipeline.address, pipeline.instruction), "\n"
  );
}

auto RSP::powerCore() -> void {
  for(uint n : range(32)) GPR[n].u32 = 0;
  PC = 0;
  branch.reset();
  for(uint n : range(32)) VPR[n].u128 = 0;
  ACCH = zero;
  ACCM = zero;
  ACCL = zero;
  VCOH = zero;
  VCOL = zero;
  VCCH = zero;
  VCCL = zero;
  VCE  = zero;
  DIVIN  = 0;
  DIVOUT = 0;
  DIVDP  = 0;

  reciprocals[0] = u16(~0);
  for(u16 index : range(1, 512)) {
    u64 a = index + 512;
    u64 b = (u64(1) << 34) / a;
    reciprocals[index] = u16(b + 1 >> 8);
  }

  for(u16 index : range(0, 512)) {
    u64 a = index + 512 >> (index % 2 == 1);
    u64 b = 1 << 17;
    //find the largest b where b < 1.0 / sqrt(a)
    while(a * (b + 1) * (b + 1) < (u64(1) << 44)) b++;
    inverseSquareRoots[index] = u16(b >> 1);
  }
}
