HitachiDSP hitachidsp;
#include "memory.cpp"
#include "serialization.cpp"

auto HitachiDSP::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "Hitachi");
  from = Node::scan(parent = node, from);
}

auto HitachiDSP::unload() -> void {
  node = {};

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
