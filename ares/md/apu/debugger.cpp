auto APU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("APU RAM");
  memory.ram->setSize(8_KiB);
  memory.ram->setRead([&](u32 address) -> u8 {
    return apu.ram.read(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return apu.ram.write(address, data);
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "APU");
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "APU");
}

auto APU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(apu.PC)) {
    tracer.instruction->notify(apu.disassembleInstruction(), apu.disassembleContext());
  }
}

auto APU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
