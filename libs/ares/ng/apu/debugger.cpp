auto APU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("APU Work RAM");
  memory.ram->setSize(apu.ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return apu.ram[address];
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    apu.ram[address] = data;
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
  if(unlikely(tracer.instruction->enabled())) {
    if(tracer.instruction->address(apu.PC)) {
      tracer.instruction->notify(apu.disassembleInstruction(), apu.disassembleContext());
    }
  }
}

auto APU::Debugger::interrupt(string_view type) -> void {
  if(unlikely(tracer.interrupt->enabled())) {
    tracer.interrupt->notify(type);
  }
}
