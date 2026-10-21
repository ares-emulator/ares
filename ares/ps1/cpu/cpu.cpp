#include <ps1/ps1.hpp>
#include <bit>

namespace ares::PlayStation {

CPU cpu;
#include "delay-slots.cpp"
#include "write-queue.cpp"
#include "memory.cpp"
#include "icache.cpp"
#include "exceptions.cpp"
#include "breakpoints.cpp"
#include "interpreter.cpp"
#include "interpreter-ipu.cpp"
#include "interpreter-scc.cpp"
#include "interpreter-gte.cpp"
#include "debugger.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  ram.allocate(2_MiB);
  scratchpad.allocate(1_KiB);
  gte.constructTable();
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger = {};
  scratchpad.reset();
  ram.reset();
  node.reset();
}

alwaysinline auto CPU::instructionPrologue(u32 instruction) -> void {
  pipeline.address = ipu.pc;
  pipeline.instruction = instruction;
  if(unlikely(ipu.pc == 0xa0 || ipu.pc == 0xb0)) {
    if(unlikely(debugger.tracer.message->enabled())) debugger.message();
  }
  if(unlikely(debugger.tracer.instruction->enabled())) debugger.instruction();
}

alwaysinline auto CPU::instructionEpilogue() -> void {
  ipu.pb = ipu.pc;
  ipu.pc = ipu.pd;
  ipu.pd = ipu.pd + 4;

  if(unlikely(delay.loadPending)) processDelayLoad();
  if(unlikely(delay.branchPending)) processDelayBranch();
  ipu.r[0] = 0;

  if(unlikely(exception.interruptsPending())) {
    debugger.interrupt(scc.cause.interruptPending);
    exception.interrupt();
  }
  if(unlikely(exception.triggered)) exception.triggered = 0;
}

auto CPU::main() -> void {
  //we need to return periodically to allow save states/exit/etc to function
  //re-entering has high function call overhead; but we can batch instructions
  while(true) {
    if constexpr(Accuracy::CPU::Breakpoints) {
      if(unlikely(breakpoint.active) && unlikely(breakpoint.testCode(ipu.pc))) {
        return (void)instructionEpilogue();
      }
    }

    if constexpr(Accuracy::CPU::AddressErrors) {
      if(unlikely(ipu.pc & 3)) {
        exception.address<Read>(ipu.pc);
        return (void)instructionEpilogue();
      }
    }

    u32 instruction = fetch(ipu.pc);
    if(unlikely(exception())) return (void)instructionEpilogue();

    instructionPrologue(instruction);
    decoderEXECUTE();
    instructionEpilogue();

    if constexpr(lockstep) {
      if(unlikely(++instructionsSinceYield >= 512 || ipu.pb + 4 != ipu.pc)) {
        instructionsSinceYield = 0;
        synchronize();
        break;
      }
    } else if(unlikely(++instructionsSinceYield >= 512)) {
      instructionsSinceYield = 0;
      synchronize();
      break;
    }
  }
}

auto CPU::stepSlow(u32 clocks) -> void {
  if(clocks == 0) return;

  if(cyclesUntilForcedSync <= 0) resetNextEvent();

  while(clocks > 0) {
    u32 stepClocks = clocks;
    if constexpr(!lockstep) stepClocks = min<u32>(stepClocks, cyclesUntilForcedSync);
    if(writeQueue.count | writeQueue.remaining) stepClocks = writeQueueWindow(stepClocks);

    mulDivRemaining -= min(mulDivRemaining, stepClocks);
    accruedCycles += stepClocks;
    clocks -= stepClocks;
    cyclesUntilForcedSync -= stepClocks;

    if(writeQueue.remaining) writeQueueAdvance(stepClocks);
    if(lockstep || writeQueue.stalled || cyclesUntilForcedSync <= 0) synchronize();
  }
}

auto CPU::synchronize() -> void {
  if(accruedCycles == 0) return;
  resetNextEvent();
  timer.step(accruedCycles);
  disc.step(accruedCycles);
  peripheral.step(accruedCycles);
  if(u32 clocks = disc.nextEvent()) setNextEvent(clocks);
  if(u32 clocks = peripheral.nextEvent()) setNextEvent(clocks);
  if(unlikely(expansion.connected())) expansion.step(accruedCycles);
  Thread::step(accruedCycles);
  accruedCycles = 0;
  Thread::synchronize();
}

auto CPU::resetNextEvent() -> void {
  cyclesUntilForcedSync = gpu.pixelAccuracySetting->value() ? forceSyncInterval : relaxedSyncInterval;
}

auto CPU::setNextEvent(u32 clocks) -> void {
  if(cyclesUntilForcedSync <= 0) resetNextEvent();
  cyclesUntilForcedSync = min<s32>(cyclesUntilForcedSync, (s32)max<u32>(1, clocks));
}

auto CPU::ioSynchronize() -> void {
  synchronize();
}

auto CPU::waitDMA() -> void {
  while(dma.active()) {
    step(min<u32>(dmaStallGranularity, dma.cpuReleaseWindow()));
    synchronize();
  }
}

auto CPU::idle(u32 clocks) -> void {
  while(writeQueue.count | writeQueue.remaining) {
    step(writeQueue.remaining ? writeQueue.remaining : 1);
  }
  if(clocks == 0) {
    clocks = cyclesUntilForcedSync > 0 ? (u32)cyclesUntilForcedSync : (u32)(gpu.pixelAccuracySetting->value() ? forceSyncInterval : relaxedSyncInterval);
  }
  step(clocks);
  synchronize();
}

auto CPU::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&CPU::main, this));
  random.array({ram.data, ram.size});
  random.array({scratchpad.data, scratchpad.size});

  accruedCycles = 0;
  resetNextEvent();
  instructionsSinceYield = 0;
  mulDivRemaining = 0;
  writeQueue = {};
  busAccess = false;
  ioPoll = {};

  pipeline = {};
  delay = {};
  icache.power(reset);
  fastBooted = 0;
  exception.triggered = 0;
  breakpoint.lastPC = 0;
  breakpoint.active = false;
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
}

}
