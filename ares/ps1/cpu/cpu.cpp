#include <ps1/ps1.hpp>

namespace ares::PlayStation {

CPU cpu;
#include "delay-slots.cpp"
#include "memory.cpp"
#include "cache.cpp"
#include "exceptions.cpp"
#include "breakpoints.cpp"
#include "ipu.cpp"
#include "scc.cpp"
#include "gte.cpp"
#include "decoder.cpp"
#include "recompiler.cpp"
#include "debugger.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  ram.allocate(2_MiB);
  ram.setWaitStates(4, 4, 4);
  scratchpad.allocate(1_KiB);
  scratchpad.setWaitStates(0, 0, 0);
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger = {};
  scratchpad.reset();
  ram.reset();
  node.reset();
}

auto CPU::main() -> void {
  instruction();
  synchronize();
}

auto CPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto CPU::synchronize() -> void {
  timer.step(Thread::clock);
  gpu.clock -= Thread::clock;
  dma.clock -= Thread::clock;
  disc.clock -= Thread::clock;
  spu.clock -= Thread::clock;
  peripheral.clock -= Thread::clock;
  Thread::clock = 0;

  while(gpu.clock < 0) gpu.main();
  while(dma.clock < 0) dma.main();
  while(disc.clock < 0) disc.main();
  while(spu.clock < 0) spu.main();
  while(peripheral.clock < 0) peripheral.main();
}

auto CPU::instruction() -> void {
  if constexpr(Accuracy::CPU::Interpreter) {
    if constexpr(Accuracy::CPU::Breakpoints) {
      if(unlikely(breakpoint.testCode(ipu.pc))) {
        return (void)instructionEpilogue();
      }
    }

    if constexpr(Accuracy::CPU::AddressErrors) {
      if(unlikely(ipu.pc & 3)) {
        exception.address<Read>(ipu.pc);
        return (void)instructionEpilogue();
      }
    }

    pipeline.address = ipu.pc;
    pipeline.instruction = fetch(ipu.pc);
    if(exception()) return (void)instructionEpilogue();

    debugger.instruction();
  //instructionDebug();
    decoderEXECUTE();
    instructionEpilogue();
  }

  if constexpr(Accuracy::CPU::Recompiler) {
    auto block = recompiler.block(ipu.pc);
    block->execute();
  }
}

auto CPU::instructionEpilogue() -> bool {
  if constexpr(Accuracy::CPU::Recompiler) {
    icache.step(ipu.pc);  //simulates timings without performing actual icache loads
  }

  ipu.pb = ipu.pc;
  ipu.pc = ipu.pd;
  ipu.pd = ipu.pd + 4;

  processDelayLoad();
  processDelayBranch();
  ipu.r[0] = 0;  //it's faster to allow assigning to r0 and then clearing it later

  if(auto interrupts = exception.interruptsPending()) {
    debugger.interrupt(scc.cause.interruptPending);
    exception.interrupt();
  }
  exception.triggered = 0;

  //the recompiler needs to know when branches occur to break execution of blocks early
  if(ipu.pb + 4 != ipu.pc) {
    debugger.message();
    debugger.function();
    return true;
  }
  return false;
}

auto CPU::instructionHook() -> void {
  //fast-boot or executable side-loading
  if(ipu.pd == 0x8003'0000) {
    if(!disc.cd || disc.audioCD()) {
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

        ipu.pd = pc;
        ipu.r[28] = gp;
        for(uint address : range(exe.size - source)) {
          ram.writeByte(target + address, exe.readByte(source + address));
        }
      }
    } else if(system.fastBoot->value()) {
      ipu.pd = ipu.r[31];
    }
  }
}

auto CPU::instructionDebug() -> void {
  if constexpr(Accuracy::CPU::Recompiler) {
    pipeline.address = ipu.pc;
    pipeline.instruction = bus.read<Word>(pipeline.address);
  }

  static vector<bool> mask;
  if(!mask) mask.resize(0x0800'0000);
  if(mask[ipu.pc >> 2 & 0x07ff'ffff]) return;
  mask[ipu.pc >> 2 & 0x07ff'ffff] = 1;

  static uint counter = 0;
//if(++counter > 100) return;
  print(
    disassembler.hint(hex(pipeline.address, 8L)), "  ",
    disassembler.hint(hex(pipeline.instruction, 8L)), "  ",
    disassembler.disassemble(pipeline.address, pipeline.instruction), "\n"
  );
}

auto CPU::power(bool reset) -> void {
  Thread::reset();
  icache.power(reset);

  ipu.pc = 0xbfc0'0000;
  ipu.pd = ipu.pc + 4;
  scc.status.enable.coprocessor0 = 1;
  scc.status.enable.coprocessor2 = 1;

  gte.constructTable();

  if constexpr(Accuracy::CPU::Recompiler) {
    recompiler.allocator.resize(512_MiB, bump_allocator::executable | bump_allocator::zero_fill);
    recompiler.reset();
  }
}

}
