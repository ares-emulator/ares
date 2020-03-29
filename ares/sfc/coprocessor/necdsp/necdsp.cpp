NECDSP necdsp;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto NECDSP::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "NEC");
  from = Node::scan(parent = node, from);

  debugger.load(parent, from);
}

auto NECDSP::unload() -> void {
  debugger = {};
  node = {};

  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto NECDSP::main() -> void {
  debugger.instruction();
  exec();

  Thread::step(1);
  Thread::synchronize(cpu);
}

auto NECDSP::power() -> void {
  uPD96050::power();
  Thread::create(Frequency, {&NECDSP::main, this});
  cpu.coprocessors.append(this);
}
