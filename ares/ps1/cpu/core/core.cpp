#include "scc-registers.cpp"
#include "memory.cpp"
#include "decoder.cpp"
#include "gte.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "gte-instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

#define PC core.pc

auto CPU::raiseException(uint code, uint coprocessor) -> void {
  if(debugger.tracer.exception->enabled()) {
    if(code != 0) debugger.exception(hex(code, 2L));
  }

  scc.status.frame[2] = scc.status.frame[1];
  scc.status.frame[1] = scc.status.frame[0];
  scc.status.frame[0] = {};

  scc.epc = PC;
  scc.cause.exceptionCode = code;
  scc.cause.coprocessorError = coprocessor;
  if(scc.cause.branchDelay = delay.branch.inDelaySlot()) scc.epc -= 4;

  PC = !scc.status.vectorLocation ? 0x8000'0080 : 0xbfc0'0180;
  delay.branch.exception();
}

auto CPU::instruction() -> void {
  auto address = PC;

  if(auto interrupts = scc.cause.interruptPending & scc.status.interruptMask) {
    if(interrupt.line & scc.status.frame[0].interruptEnable) {
      if(debugger.tracer.interrupt->enabled()) {
        debugger.interrupt(hex(scc.cause.interruptPending, 2L));
      }
      scc.cause.interruptPending = interrupts;
      step(1);
      return raiseException(0);
    }
  }

  if constexpr(1) {
    if constexpr(Accuracy::CPU::AlignmentErrors) {
      if(unlikely(address & 3)) {
        exception.busInstruction();
        step(2);
        return;
      }
    }

    pipeline.address = address;
    pipeline.instruction = bus.readWord(address);
    debugger.instruction();
  //instructionDebug();
    decoderEXECUTE();
    instructionEpilogue();
    step(2);
  }
}

auto CPU::instructionEpilogue() -> bool {
  if(delay.load.target) {
    *delay.load.target &= delay.load.mask;
    *delay.load.target |= delay.load.source;
    delay.load.target = nullptr;
  }

  if(delay.fetch.target) {
    delay.load = delay.fetch;
    delay.fetch.target = nullptr;
  }

  if(delay.write.target) {
    *delay.write.target = delay.write.source;
    delay.write.target = nullptr;
  }

  core.r[0] = 0;

  switch(delay.branch.state) {
  case Delay::Branch::Step:
    PC += 4;
    return 0;
  case Delay::Branch::Take:
    PC += 4;
    delay.branch.delaySlot();
    return 0;
  case Delay::Branch::DelaySlot:
    PC = delay.branch.pc;
    delay.branch.reset();
    instructionHook();
    return 1;
  case Delay::Branch::Exception:
    delay.branch.reset();
    return 1;
  }

  unreachable;
}

auto CPU::instructionHook() -> void {
  //fast-boot or executable side-loading
  if(PC == 0x8003'0000) {
    if(!disc.cd || disc.audio()) {
      //todo: is it possible to fast boot into the BIOS menu here?
    } else if(disc.executable()) {
      if(auto fp = platform->open(disc.node, "program.exe", File::Read, File::Required)) {
        Memory::Readable exe;
        exe.allocate(fp->size());
        exe.load(fp);
        u32 pc     = exe.readWord(0x10);
        u32 gp     = exe.readWord(0x14);
        u32 target = exe.readWord(0x18) & ram.size - 1;
        u32 source = 2048;

        PC = pc;
        core.r[28] = gp;
        for(uint address : range(exe.size - source)) {
          ram.writeByte(target + address, exe.readByte(source + address));
        }
      }
    } else if(system.fastBoot->value()) {
      PC = core.r[31];
    }
  }

  if(debugger.tracer.function->enabled()) {
    debugger.function();
  }

  if(debugger.tracer.message->enabled()) {
    if(PC == 0xa0 && core.r[9] == 0x3c) {
      debugger.message((char)core.r[4]);
    }

    if(PC == 0xb0 && core.r[9] == 0x3d) {
      debugger.message((char)core.r[4]);
    }
  }
}

auto CPU::instructionDebug() -> void {
//pipeline.address = PC;
//pipeline.instruction = bus.readWord(pipeline.address);

  static vector<bool> mask;
  if(!mask) mask.resize(0x0800'0000);
  if(mask[(PC & 0x1fff'ffff) >> 2]) return;
  mask[(PC & 0x1fff'ffff) >> 2] = 1;

  static uint counter = 0;
//if(++counter > 100) return;
  print(
    disassembler.hint(hex(pipeline.address, 8L)), "  ",
    disassembler.hint(hex(pipeline.instruction, 8L)), "  ",
    disassembler.disassemble(pipeline.address, pipeline.instruction), "\n"
  );
}

#undef PC

auto CPU::powerCore(bool reset) -> void {
  for(uint n : range(32)) core.r[n] = 0;
  core.lo = 0;
  core.hi = 0;
  core.pc = 0xbfc0'0000;
  delay = {};
  gte.constructTable();
}
