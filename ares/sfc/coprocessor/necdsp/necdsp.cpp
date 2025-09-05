NECDSP necdsp;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto NECDSP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("NEC");

  debugger.load(node);
}

auto NECDSP::unload() -> void {
  debugger = {};
  node = {};

  std::erase(cpu.coprocessors, this);
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
  cpu.coprocessors.push_back(this);
}
