HitachiDSP hitachidsp;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto HitachiDSP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("Hitachi");

  debugger.load(node);
}

auto HitachiDSP::unload() -> void {
  node = {};
  debugger = {};

  rom.reset();
  ram.reset();
  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto HitachiDSP::step(uint clocks) -> void {
  HG51B::step(clocks);
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto HitachiDSP::halt() -> void {
  HG51B::halt();
  if(io.irq == 0) cpu.irq(r.i = 1);
}

auto HitachiDSP::power() -> void {
  HG51B::power();
  Thread::create(Frequency, [&] { main(); });
  cpu.coprocessors.append(this);
}
