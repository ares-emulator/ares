auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.wram = parent->append<Node::Memory>("CPU WRAM");
  memory.wram->setSize(128_KiB);
  memory.wram->setRead([&](uint32 address) -> uint8 {
    return cpu.wram[uint17(address)];
  });
  memory.wram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.wram[uint17(address)] = data;
  });

  tracer.instruction = parent->append<Node::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.r.pc.d)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext(), {
      "V:", pad(cpu.vcounter(), 3L), " ", "H:", pad(cpu.hcounter(), 4L), " I:", (uint)cpu.field()
    });
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
