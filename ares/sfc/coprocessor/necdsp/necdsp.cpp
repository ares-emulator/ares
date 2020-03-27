NECDSP necdsp;
#include "memory.cpp"
#include "serialization.cpp"

auto NECDSP::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "NEC");
  from = Node::scan(parent = node, from);

  debugInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "NEC");
  debugInstruction->setAddressBits(14);
}

auto NECDSP::unload() -> void {
  debugInstruction = {};
  node = {};

  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto NECDSP::main() -> void {
  if(debugInstruction->enabled() && debugInstruction->address(regs.pc)) {
    debugInstruction->notify(disassembleInstruction(), disassembleContext());
  }
  exec();

  Thread::step(1);
  Thread::synchronize(cpu);
}

auto NECDSP::power() -> void {
  uPD96050::power();
  Thread::create(Frequency, {&NECDSP::main, this});
  cpu.coprocessors.append(this);
}
