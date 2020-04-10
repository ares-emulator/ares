ARMDSP armdsp;
#include "memory.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto ARMDSP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("ARM");

  debugger.load(node);
}

auto ARMDSP::unload() -> void {
  debugger = {};
  node = {};

  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto ARMDSP::boot() -> void {
  //reset hold delay
  while(bridge.reset) {
    step(1);
    continue;
  }

  //reset sequence delay
  if(bridge.ready == false) {
    step(65'536);
    bridge.ready = true;
  }
}

auto ARMDSP::main() -> void {
  processor.cpsr.t = 0;  //force ARM mode
  debugger.instruction();
  instruction();
}

auto ARMDSP::step(uint clocks) -> void {
  if(bridge.timer && --bridge.timer == 0);
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto ARMDSP::power() -> void {
  random.array((uint8*)programRAM, sizeof(programRAM));
  bridge.reset = false;
  reset();
}

auto ARMDSP::reset() -> void {
  ARM7TDMI::power();
  Thread::create(Frequency, [&] {
    boot();
    while(true) scheduler.synchronize(), main();
  });
  cpu.coprocessors.append(this);

  bridge.ready = false;
  bridge.signal = false;
  bridge.timer = 0;
  bridge.timerlatch = 0;
  bridge.cputoarm.ready = false;
  bridge.armtocpu.ready = false;
}
