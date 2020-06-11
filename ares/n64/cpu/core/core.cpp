#include "registers.hpp"
#include "scc-tlb.cpp"
#include "scc-registers.cpp"
#include "fpu-registers.cpp"
#include "memory.cpp"
#include "decoder.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "fpu-instructions.cpp"
#include "recompiler.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto CPU::raiseException(uint code, uint coprocessor) -> void {
  if(debugger.tracer.exception->enabled()) {
    if(code != 0) debugger.exception(hex(code, 2L));
  }

  if(!scc.status.exceptionLevel) {
    scc.epc = PC;
    scc.status.exceptionLevel = 1;
    scc.cause.exceptionCode = code;
    scc.cause.coprocessorError = coprocessor;
    if(scc.cause.branchDelay = branch.inDelaySlot()) scc.epc -= 4;
  } else {
    scc.cause.exceptionCode = code;
    scc.cause.coprocessorError = coprocessor;
  }

  u64 vectorBase = !scc.status.vectorLocation ? i32(0x8000'0000) : i32(0xbfc0'0200);
  u32 vectorOffset = (code == 2 || code == 3) ? 0x0000 : 0x0180;
  PC = vectorBase + vectorOffset;
  branch.exception();
  context.setMode();
}

auto CPU::instruction() -> void {
  if(auto interrupts = scc.cause.interruptPending & scc.status.interruptMask) {
    if(scc.status.interruptEnable && !scc.status.exceptionLevel && !scc.status.errorLevel) {
      if(debugger.tracer.interrupt->enabled()) {
        debugger.interrupt(hex(scc.cause.interruptPending, 2L));
      }
      scc.cause.interruptPending = interrupts;
      step(2);
      return raiseException(0, interrupts);
    }
  }

  debugger.instruction();
//for(auto& block : blocks) block.reset();
//blocks.reset();
//maybe<Block&> block;
//if(branch.state == Branch::Step && (block = recompile(PC))) {
//  block->execute();
//  step(block->step);
//} else {
    pipeline.address = PC;
    pipeline.instruction = readWord(pipeline.address)(0);
  //instructionDEBUG();
    instructionEXECUTE();
    instructionEpilogue();
    step(2);
//}
}

auto CPU::instructionEpilogue() -> void {
  GPR[0].u64 = 0;

  switch(branch.state) {
  case Branch::Step: PC += 4; break;
  case Branch::DelaySlot: PC = branch.pc; branch.reset(); break;
  case Branch::Take: PC += 4; branch.delaySlot(); break;
  case Branch::Exception: branch.reset(); break;
  case Branch::Discard: PC += 8; branch.reset(); break;
  }

  if(--scc.random.index < scc.wired.index) {
    scc.random.index = 31;
  }
}

auto CPU::instructionDEBUG() -> void {
  pipeline.address = PC;
  pipeline.instruction = readWord(pipeline.address)(0);

  static vector<bool> mask;
  if(!mask) mask.resize(0x08000000);
  if(mask[(PC & 0x1fffffff) >> 2]) return;
  mask[(PC & 0x1fffffff) >> 2] = 1;

  static uint counter = 0;
//if(++counter > 100) return;
  print(
    disassembler.hint(hex(pipeline.address, 8L)), "  ",
  //disassembler.hint(hex(pipeline.instruction, 8L)), "  ",
    disassembler.disassemble(pipeline.address, pipeline.instruction), "\n"
  );
}

auto CPU::powerR4300(bool reset) -> void {
  for(uint n : range(32)) GPR[n].u64 = 0;
  scc = {};
  scc.status.softReset = 1;  //reset;
  fpu = {};
  LO.u64 = 0;
  HI.u64 = 0;
  GPR[Core::Register::SP].u64 = i32(0xa400'1ff0);
  PC = i32(0xbfc00000);
  branch.reset();
  fesetround(FE_TONEAREST);
  context.setMode();
}

auto CPU::Context::setMode() -> void {
  mode = min(2, self.scc.status.privilegeMode);
  if(self.scc.status.exceptionLevel) mode = Mode::Kernel;
  if(self.scc.status.errorLevel) mode = Mode::Kernel;

  segment[0] = Segment::Mapped;
  segment[1] = Segment::Mapped;
  segment[2] = Segment::Mapped;
  segment[3] = Segment::Mapped;
  if(mode == Mode::Kernel) {
    segment[4] = Segment::Cached;
    segment[5] = Segment::Uncached;
    segment[6] = Segment::Mapped;
    segment[7] = Segment::Mapped;
  }
  if(mode == Mode::Supervisor) {
    segment[4] = Segment::Invalid;
    segment[5] = Segment::Invalid;
    segment[6] = Segment::Mapped;
    segment[7] = Segment::Invalid;
  }
  if(mode == Mode::User) {
    segment[4] = Segment::Invalid;
    segment[5] = Segment::Invalid;
    segment[6] = Segment::Invalid;
    segment[7] = Segment::Invalid;
  }
}
