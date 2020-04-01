auto CPU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  string origin = Model::SuperGameBoy() ? "SGB" : "CPU";

  memory.wram = Node::append<Node::Memory>(parent, from, "CPU WRAM");
  memory.wram->setSize(cpu.wram.size());
  memory.wram->setRead([&](uint32 address) -> uint8 {
    return cpu.wram[address];
  });
  memory.wram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.wram[address] = data;
  });

  memory.hram = Node::append<Node::Memory>(parent, from, "CPU HRAM");
  memory.hram->setSize(cpu.hram.size());
  memory.hram->setRead([&](uint32 address) -> uint8 {
    return cpu.hram[address];
  });
  memory.hram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.hram[address] = data;
  });

  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", origin);
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", origin);
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.PC)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
