#include <ps1/ps1.hpp>
#include <bit>
#include <array>

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
  u32 opcode = instruction >> 26;
  if(opcode >= 0x20 && opcode <= 0x26 && (instruction >> 16 & 31) == readTarget) readTarget = 0;
  if(unlikely(ipu.pc == 0xa0 || ipu.pc == 0xb0)) {
    if(unlikely(debugger.tracer.message->enabled())) debugger.message();
  }
  if(unlikely(debugger.tracer.instruction->enabled())) debugger.instruction();
}

alwaysinline auto CPU::instructionEpilogue() -> void {
  ipu.pb = ipu.pc;
  ipu.pc = ipu.pd;
  ipu.pd = ipu.pd + 4;

  if(unlikely(delay.loadPending | delay.branchPending)) {
    if(delay.loadPending) processDelayLoad();
    if(delay.branchPending) processDelayBranch();
  }
  ipu.r[0] = 0;

  if(unlikely(exception.interruptsPending())) {
    debugger.interrupt(scc.cause.interruptPending);
    exception.interrupt();
  }
  if(unlikely(exception.triggered)) exception.triggered = 0;
}

static constexpr auto controlFlowClass = []() {
  std::array<u8, 64> t{};
  for(u32 o = 1; o <= 7; o++) t[o] = 1;
  t[0x00] = 2;
  t[0x10] = 3;
  t[0x12] = 3;
  return t;
}();

alwaysinline auto CPU::fetchAhead(u32 address, PrefetchedInstruction& slot) -> void {
  if(likely(!(address & 3) && (address & 0x7fff'ffff) <= memory.ramLimit)) {
    slot.address = address;
    slot.instruction = fetch(address);
    return;
  }
  u32 physical = address & 0x1fff'ffff;
  if((address & 3) || (address >> 29 != 0 && address >> 29 != 4 && address >> 29 != 5)
  || (physical > memory.ramLimit && physical < 0x1fc0'0000)) {
    slot = {};
    return;
  }
  slot.address = address;
  slot.instruction = fetch(address);
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

    u32 instruction;
    if(prefetched[0].address == ipu.pc) instruction = prefetched[0].instruction;
    else {
      prefetched[0] = prefetched[1] = {};
      instruction = fetch(ipu.pc);
    }
    if(unlikely(exception())) return (void)instructionEpilogue();
    if(unlikely(readRemaining)) {
      u32 opcode = instruction >> 26;
      if((opcode >= 0x20 && opcode <= 0x26) || (opcode >= 0x30 && opcode <= 0x33)) step(readRemaining);
    }
    if(unlikely((readLatency | readWritebackRemaining) && readTarget)) readInterlock(instruction);
    instructionPrologue(instruction);
    u32 opcode = instruction >> 26;
    u32 flowClass = controlFlowClass[opcode];
    bool controlFlow = flowClass == 1
                    || (flowClass == 2 && ((0x300ull >> (instruction & 63)) & 1))
                    || (flowClass == 3 && (instruction >> 21 & 31) == 8);
    if(likely(!controlFlow)) {
      if(prefetched[1].address == ipu.pd) prefetched[0] = prefetched[1];
      else fetchAhead(ipu.pd, prefetched[0]);
      fetchAhead(ipu.pd + 4, prefetched[1]);
      if(unlikely(exception())) return (void)instructionEpilogue();
    }
    decoderEXECUTE();
    if(unlikely(controlFlow)) {
      if(unlikely(exception())) return (void)instructionEpilogue();
      if(prefetched[1].address == ipu.pd) prefetched[0] = prefetched[1];
      else fetchAhead(ipu.pd, prefetched[0]);
      fetchAhead(delay.branch[1].take ? (u32)delay.branch[1].address : ipu.pd + 4, prefetched[1]);
      if(unlikely(exception())) return (void)instructionEpilogue();
    }
    instructionEpilogue();

    if(unlikely(++instructionsSinceYield >= 512)) {
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
    if(readRemaining) stepClocks = min(stepClocks, readRemaining);
    stepClocks = min<u32>(stepClocks, cyclesUntilForcedSync);
    if(writeQueue.count | writeQueue.remaining) stepClocks = writeQueueWindow(stepClocks);

    mulDivRemaining -= min(mulDivRemaining, stepClocks);
    gte.remaining -= min(gte.remaining, stepClocks);
    readLatency -= min(readLatency, stepClocks);
    readWritebackRemaining -= min(readWritebackRemaining, stepClocks);
    accruedCycles += stepClocks;
    clocks -= stepClocks;
    cyclesUntilForcedSync -= stepClocks;

    if(writeQueue.remaining) writeQueueAdvance(stepClocks);
    if(readRemaining) {
      readRemaining -= stepClocks;
      if(!readRemaining) busAccess = false;
    }
    if(writeQueue.stalled || cyclesUntilForcedSync <= 0) synchronize();
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

auto CPU::dmaStallSlices(u32 slice) -> u32 {
  if(readRemaining || cyclesUntilForcedSync <= (s32)slice * 2 || timer.sysclockGated()) return 1;
  u64 now = clock() + (u64)accruedCycles * scalar();
  if(dma.clock() < now) return 1;
  u64 slices = (dma.clock() - now) / ((u64)slice * scalar());
  return max<u64>(1, min<u64>(slices, ((u64)cyclesUntilForcedSync - 1) / slice));
}

auto CPU::waitDMA() -> void {
  while(dma.active()) {
    u32 slice = min<u32>(dmaStallGranularity, dma.cpuReleaseWindow());
    u32 clocks = slice * dmaStallSlices(slice);
    if(clocks > slice && (writeQueue.count | writeQueue.remaining)) {
      if(!busAccess) writeQueue.page = ~0u;
      writeQueue.stalled = !busAccess;
      advance(clocks);
    } else {
      step(clocks);
    }
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
  gte.remaining = 0;
  gte.command = 0;
  writeQueue = {};
  busAccess = false;
  busTransactionClocks = readRemaining = readLatency = readWritebackRemaining = readTarget = 0;
  ioPoll = {};

  pipeline = {};
  prefetched[0] = prefetched[1] = {};
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
