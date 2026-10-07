#include <ps1/ps1.hpp>

namespace ares::PlayStation {

CPU cpu;
#include "debugger/debugger.cpp"
#include "delay-slots.cpp"
#include "exceptions.cpp"
#include "execution.cpp"
#include "gte/gte.cpp"
#include "icache.cpp"
#include "interpreter/interpreter.cpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "scc/scc.cpp"
#include "serialization.cpp"
#include "write-buffer.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  ram.allocate(2_MiB);
  scratchpad.allocate(1_KiB);
  gte.constructTable();
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger.unload();
  scratchpad.reset();
  ram.reset();
  node.reset();
}

auto CPU::main() -> void {
  // I/O may repeatedly clear accruedCycles before a branch reaches the cooldown.
  // Bound the batch so synchronized saves always reach Thread::Enter's safe point.
  for(u32 instructions : range(1024)) {
    instruction();
    if(ipu.pb + 4 != ipu.pc) {
      if(accruedCycles >= branchCooldownCycles) {
        synchronize();
        return;
      }
    }
  }
  synchronize();
}

auto CPU::step(u32 clocks) -> void {
  if(clocks == 0) return;

  execution.clock += clocks;
  retireExecutionUnits();
  retireMemoryFrontend();
  accruedCycles += clocks;

  if(cyclesUntilForcedSync <= 0) {
    cyclesUntilForcedSync = forceSyncInterval;
  }

  cyclesUntilForcedSync -= (s32)clocks;

  if(cyclesUntilForcedSync > 0) return;
  synchronize();
}

auto CPU::synchronize() -> void {
  if(accruedCycles == 0) return;
  timer.step(accruedCycles);
  disc.step(accruedCycles);
  peripheral.step(accruedCycles);
  Thread::step(accruedCycles);
  Thread::synchronize();

  accruedCycles = 0;
  cyclesUntilForcedSync = 0;
}

auto CPU::ioSynchronize() -> void {
  if(accruedCycles >= ioCooldownCycles) synchronize();
}

auto CPU::waitBus(u8 owner) -> void {
  //Let devices reach the elapsed CPU time before competing for the next bus word.
  if(active()) synchronize();
  while(!bus.acquire(owner)) {
    step(1);
    if(active()) synchronize();
  }
}

auto CPU::stepBus(u32 clocks, u8 owner, bool ramData) -> void {
  bool codeDataContention = ramData && memory.ram.delay
    && bus.arbiter.owner == Bus::InstructionRefill;
  waitBus(owner);
  step(clocks + codeDataContention);
  bus.release(owner);
}

auto CPU::instructionHook() -> void {
  //fast-boot or executable side-loading
  if(ipu.pd == 0x8003'0000 && !exeLoaded) {
    exeLoaded = 1;
    if(!disc.cd || disc.audioCD()) {
      //todo: is it possible to fast boot into the BIOS menu here?
    } else if(exe.size > 0) {
        u32 pc     = exe.readWord(0x10);
        u32 gp     = exe.readWord(0x14);
        u32 target = exe.readWord(0x18) & ram.size - 1;
        u32 source = 2048;

        ipu.pd = pc;
        ipu.r[28] = gp;
        for(u32 address : range(exe.size - source)) {
          ram.writeByte(target + address, exe.readByte(source + address));
        }
    } else if(system.fastBoot->value()) {
      ipu.pd = ipu.r[31];
    }
  }
}

auto CPU::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&CPU::main, this));
  random.array({ram.data, ram.size});
  random.array({scratchpad.data, scratchpad.size});

  accruedCycles = 0;
  cyclesUntilForcedSync = 0;
  execution = {};
  frontend = {};

  pipeline = {};
  delay = {};
  icache.power(reset);
  exeLoaded = 0;
  exception.triggered = 0;
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
  gte.power();
}

}
