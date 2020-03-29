auto CPU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  memory.wram = Node::append<Node::Memory>(parent, from, "CPU WRAM");
  memory.wram->setSize(128_KiB);
  memory.wram->setRead([&](uint32 address) -> uint8 {
    return cpu.wram[uint17(address)];
  });
  memory.wram->setWrite([&](uint32 address, uint8 data) -> void {
    cpu.wram[uint17(address)] = data;
  });

  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "CPU");
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
