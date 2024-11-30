#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

CPU cpu;
#include "prefetch.cpp"
#include "bus.cpp"
#include "io.cpp"
#include "memory.cpp"
#include "dma.cpp"
#include "timer.cpp"
#include "keypad.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  iwram.allocate( 32_KiB);
  ewram.allocate(256_KiB);

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::unload() -> void {
  iwram.reset();
  ewram.reset();
  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(stopped()) {
    if(!keypad.conditionMet) {
      stepIRQ();
      Thread::step(1);
      Thread::synchronize();
      return;
    }
    Thread::step(2);
    Thread::synchronize();
    context.stopped = false;
  }

  if(halted()) {
    dmaRun();
    if(!(irq.enable[0] & irq.flag[0])) {
      return step(4);
    }
    step(2);
    context.halted = false;
  }

  debugger.instruction();
  instruction();
}

auto CPU::dmaRun() -> void {
  if(!context.dmaActive) {
    context.dmaActive = true;
    while(dma[0].run() | dma[1].run() | dma[2].run() | dma[3].run());
    if(context.dmaRan) {
      idle();
      context.dmaRan = false;
      context.dmaRomAccess = false;
    }
    context.dmaActive = false;
  }
}

auto CPU::setInterruptFlag(u32 source) -> void {
  irq.flag[1] |= source;
}

inline auto CPU::stepIRQ() -> void {
  irq.synchronizer = irq.ime[0] && (irq.enable[0] & irq.flag[0]);
  irq.enable[0] = irq.enable[1];
  irq.flag[0] = irq.flag[1];
  irq.ime[0] = irq.ime[1];
}

auto CPU::step(u32 clocks) -> void {
  if(!clocks) return;

  dma[0].waiting = max(0, dma[0].waiting - (s32)clocks);
  dma[1].waiting = max(0, dma[1].waiting - (s32)clocks);
  dma[2].waiting = max(0, dma[2].waiting - (s32)clocks);
  dma[3].waiting = max(0, dma[3].waiting - (s32)clocks);

  for(auto _ : range(clocks)) {
    stepIRQ();
    timer[0].run();
    timer[1].run();
    timer[2].run();
    timer[3].run();
    timer[0].reloadLatch();
    timer[1].reloadLatch();
    timer[2].reloadLatch();
    timer[3].reloadLatch();
    if(context.timerLatched) {
      timer[0].stepLatch();
      timer[1].stepLatch();
      timer[2].stepLatch();
      timer[3].stepLatch();
      context.timerLatched = 0;
    }
    context.clock++;
  }

  Thread::step(clocks);
  Thread::synchronize(ppu, player);

  //occasionally synchronize with APU in case CPU has not recently interacted with it
  static u32 counter = 0;
  counter += clocks;
  if(counter >= 1024) {
    Thread::synchronize(apu);
    counter = 0;
  }
}

auto CPU::power() -> void {
  ARM7TDMI::power();
  Thread::create(system.frequency(), {&CPU::main, this});

  for(auto& byte : iwram) byte = 0x00;
  for(auto& byte : ewram) byte = 0x00;

  for(auto n : range(4)) dma[n] = {n};
  for(auto n : range(4)) timer[n] = {n};
  serial = {};
  keypad = {};
  joybus = {};
  irq = {};
  wait = {};
  memory = {};
  prefetch = {};
  context = {};

  dma[0].source.setBits(27); dma[0].latch.source.setBits(27);
  dma[0].target.setBits(27); dma[0].latch.target.setBits(27);
  dma[0].length.setBits(14); dma[0].latch.length.setBits(14);

  dma[1].source.setBits(28); dma[1].latch.source.setBits(28);
  dma[1].target.setBits(27); dma[1].latch.target.setBits(27);
  dma[1].length.setBits(14); dma[1].latch.length.setBits(14);

  dma[2].source.setBits(28); dma[2].latch.source.setBits(28);
  dma[2].target.setBits(27); dma[2].latch.target.setBits(27);
  dma[2].length.setBits(14); dma[2].latch.length.setBits(14);

  dma[3].source.setBits(28); dma[3].latch.source.setBits(28);
  dma[3].target.setBits(28); dma[3].latch.target.setBits(28);
  dma[3].length.setBits(16); dma[3].latch.length.setBits(16);

  for(u32 n = 0x0b0; n <= 0x0df; n++) bus.io[n] = this;  //DMA
  for(u32 n = 0x100; n <= 0x10f; n++) bus.io[n] = this;  //Timers
  for(u32 n = 0x120; n <= 0x12b; n++) bus.io[n] = this;  //Serial
  for(u32 n = 0x130; n <= 0x133; n++) bus.io[n] = this;  //Keypad
  for(u32 n = 0x134; n <= 0x15b; n++) bus.io[n] = this;  //Serial
  for(u32 n = 0x200; n <= 0x20b; n++) bus.io[n] = this;  //System
  for(u32 n = 0x300; n <= 0x303; n++) bus.io[n] = this;  //System
  //0x080-0x083 mirrored via gba/memory/memory.cpp        //System
}

}
