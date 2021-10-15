#include <ps1/ps1.hpp>

namespace ares::PlayStation {

CPU cpu;
#include "delay-slots.cpp"
#include "memory.cpp"
#include "icache.cpp"
#include "exceptions.cpp"
#include "breakpoints.cpp"
#include "interpreter.cpp"
#include "interpreter-ipu.cpp"
#include "interpreter-scc.cpp"
#include "interpreter-gte.cpp"
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
  gte.constructTable();
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

auto CPU::step(u32 clocks) -> void {
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
    decoderEXECUTE();
    instructionEpilogue();
  }

  if constexpr(Accuracy::CPU::Recompiler) {
    auto block = recompiler.block(ipu.pc);
    block->execute(*this);
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
      if(auto fp = disc.pak->read("program.exe")) {
        Memory::Readable exe;
        exe.allocate(fp->size());
        exe.load(fp);
        u32 pc     = exe.readWord(0x10);
        u32 gp     = exe.readWord(0x14);
        u32 target = exe.readWord(0x18) & ram.size - 1;
        u32 source = 2048;

        ipu.pd = pc;
        ipu.r[28] = gp;
        for(u32 address : range(exe.size - source)) {
          ram.writeByte(target + address, exe.readByte(source + address));
        }
      }
    } else if(system.fastBoot->value()) {
      ipu.pd = ipu.r[31];
    }
  }
}

auto CPU::power(bool reset) -> void {
  Thread::reset();
  ram.fill();
  scratchpad.fill();

  pipeline = {};
  delay = {};
  icache.power(reset);
  exception.triggered = 0;
  breakpoint.lastPC = 0;
  for(auto& r : ipu.r) r = 0;
  ipu.lo = 0;
  ipu.hi = 0;
  ipu.pb = 0;
  ipu.pc = 0xbfc0'0000;
  ipu.pd = ipu.pc + 4;
  scc.breakpoint = {};
  scc.targetAddress = 0;
  scc.badVirtualAddress = 0;
  scc.status = {};
  scc.cause = {};
  scc.epc = 0;
  gte.v.a.x = 0;
  gte.v.a.y = 0;
  gte.v.a.z = 0;
  gte.v.b.x = 0;
  gte.v.b.y = 0;
  gte.v.b.z = 0;
  gte.v.c.x = 0;
  gte.v.c.y = 0;
  gte.v.c.z = 0;
  gte.rgbc.r = 0;
  gte.rgbc.g = 0;
  gte.rgbc.b = 0;
  gte.rgbc.t = 0;
  gte.otz = 0;
  gte.ir.x = 0;
  gte.ir.y = 0;
  gte.ir.z = 0;
  gte.ir.t = 0;
  gte.screen[0].x = 0;
  gte.screen[0].y = 0;
  gte.screen[0].z = 0;
  gte.screen[1].x = 0;
  gte.screen[1].y = 0;
  gte.screen[1].z = 0;
  gte.screen[2].x = 0;
  gte.screen[2].y = 0;
  gte.screen[2].z = 0;
  gte.screen[3].x = 0;
  gte.screen[3].y = 0;
  gte.screen[3].z = 0;
  gte.rgb[0] = 0;
  gte.rgb[1] = 0;
  gte.rgb[2] = 0;
  gte.rgb[3] = 0;
  gte.mac.x = 0;
  gte.mac.y = 0;
  gte.mac.z = 0;
  gte.mac.t = 0;
  gte.lzcs = 0;
  gte.lzcr = 0;
  gte.rotation.a.x = 0;
  gte.rotation.a.y = 0;
  gte.rotation.a.z = 0;
  gte.rotation.b.x = 0;
  gte.rotation.b.y = 0;
  gte.rotation.b.z = 0;
  gte.rotation.c.x = 0;
  gte.rotation.c.y = 0;
  gte.rotation.c.z = 0;
  gte.translation.x = 0;
  gte.translation.y = 0;
  gte.translation.z = 0;
  gte.light.a.x = 0;
  gte.light.a.y = 0;
  gte.light.a.z = 0;
  gte.light.b.x = 0;
  gte.light.b.y = 0;
  gte.light.b.z = 0;
  gte.light.c.x = 0;
  gte.light.c.y = 0;
  gte.light.c.z = 0;
  gte.backgroundColor.r = 0;
  gte.backgroundColor.g = 0;
  gte.backgroundColor.b = 0;
  gte.color.a.r = 0;
  gte.color.a.g = 0;
  gte.color.a.b = 0;
  gte.color.b.r = 0;
  gte.color.b.g = 0;
  gte.color.b.b = 0;
  gte.color.c.r = 0;
  gte.color.c.g = 0;
  gte.color.c.b = 0;
  gte.farColor.r = 0;
  gte.farColor.g = 0;
  gte.farColor.b = 0;
  gte.ofx = 0;
  gte.ofy = 0;
  gte.h = 0;
  gte.dqa = 0;
  gte.dqb = 0;
  gte.zsf3 = 0;
  gte.zsf4 = 0;
  gte.flag.value = 0;
  gte.lm = 0;
  gte.tv = 0;
  gte.mv = 0;
  gte.mm = 0;
  gte.sf = 0;

  if constexpr(Accuracy::CPU::Recompiler) {
    auto buffer = ares::Memory::FixedAllocator::get().acquire(512_MiB);
    recompiler.allocator.resize(512_MiB, bump_allocator::executable | bump_allocator::zero_fill, buffer);
    recompiler.reset();
  }
}

}
