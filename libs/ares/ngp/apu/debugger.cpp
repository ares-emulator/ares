auto APU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("APU RAM");
  memory.ram->setSize(self.ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return self.ram[address];
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    self.ram[address] = data;
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "APU");
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "APU");
}

auto APU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.ram);
  parent->remove(tracer.instruction);
  parent->remove(tracer.interrupt);
  memory.ram.reset();
  tracer.instruction.reset();
  tracer.interrupt.reset();
}

auto APU::Debugger::instruction() -> void {
  if(likely(!tracer.instruction->enabled())) return;
  if(!tracer.instruction->address(self.PC)) return;
  tracer.instruction->notify(self.disassembleInstruction(), self.disassembleContext());
}

auto APU::Debugger::interrupt(string_view type) -> void {
  if(likely(!tracer.interrupt->enabled())) return;
  tracer.interrupt->notify(type);
}
