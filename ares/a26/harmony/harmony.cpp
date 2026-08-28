#include <a26/a26.hpp>

namespace ares::Atari2600 {

Harmony harmony;
#include "memory.cpp"
#include "timing.cpp"
#include "serialization.cpp"

auto Harmony::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Harmony ARM");
}

auto Harmony::unload() -> void {
  node = {};
  Thread::destroy();
}

auto Harmony::main() -> void {
  if(!running) return step(64);

  instruction();
  instructions++;
  if(faulted) {
    running = false;
    callPending = false;
    pendingIrqDrivenAudio = false;
    return;
  }
  if(!processor.cpsr.t) {
    if(handleTrap(pipeline.execute.address)) return;
    auto executed = callCycles;
    runs++;
    if(irqDrivenAudio) step(executed / 10);
    running = false;
    if(callPending) {
      auto pendingAudio = pendingIrqDrivenAudio;
      callPending = false;
      pendingIrqDrivenAudio = false;
      call(pendingAudio);
    }
    return;
  }
  if(instructions >= 500'000) {
    faulted = true;
    running = false;
    callPending = false;
    pendingIrqDrivenAudio = false;
  }
}

auto Harmony::power() -> void {
  Thread::create(70'000'000, std::bind_front(&Harmony::main, this));
  runs = 0;
  timer1Control = 0;
  timer1Counter = 0;
  systickControl = 4;
  systickReload = 0;
  systickCounter = 0;
  systickCalibration = 0x00abcdef;
  mamControl = 0;
  callCycles = 0;
  instructions = 0;
  running = false;
  irqDrivenAudio = false;
  callPending = false;
  pendingIrqDrivenAudio = false;
  faulted = false;
  resetProcessor();
}

auto Harmony::call(bool newIrqDrivenAudio) -> bool {
  if(running) {
    callPending = true;
    pendingIrqDrivenAudio = newIrqDrivenAudio;
    return true;
  }
  if(!resetProcessor()) return false;
  callCycles = 0;
  instructions = 0;
  running = true;
  irqDrivenAudio = newIrqDrivenAudio;
  return true;
}

auto Harmony::resetProcessor() -> bool {
  auto invocation = cartridge.armInvocation();
  if(!invocation) return false;

  ARM7TDMI::power();
  processor.cpsr.m = PSR::SYS;
  processor.cpsr.t = 1;
  processor.cpsr.f = 1;
  processor.cpsr.i = 1;
  processor.r13 = invocation.sp;
  processor.r14 = invocation.lr;
  processor.r15 = invocation.pc;
  faulted = false;
  return true;
}

auto Harmony::handleTrap(u32 address) -> bool {
  n32 value = processor.r2;
  if(!cartridge.trapARM(address, value, processor.r3)) return false;
  processor.r2 = value;
  processor.cpsr.t = 1;
  processor.r15 = processor.r14;
  return true;
}

}
