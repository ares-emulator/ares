auto APU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("APU RAM");
  memory.ram->setSize(apu.ram.size());
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return apu.ram[address];
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    apu.ram[address] = data;
  });

  tracer.instruction = parent->append<Node::Instruction>("Instruction", "APU");
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "APU");
}

auto APU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(apu.r.pc)) {
    tracer.instruction->notify(apu.disassembleInstruction(), apu.disassembleContext());
  }
}

auto APU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
