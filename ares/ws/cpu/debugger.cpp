auto CPU::Debugger::load(Node::Object parent, Node::Object from) -> void {
  memory.ram = Node::append<Node::Memory>(parent, from, "CPU RAM");
  memory.ram->setSize(SoC::ASWAN() ? 16_KiB : 64_KiB);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return iram.read(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return iram.write(address, data);
  });

  tracer.instruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  tracer.instruction->setAddressBits(20);

  tracer.interrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(uint20(cpu.V30MZ::r.cs * 16 + cpu.V30MZ::r.ip))) {
    if(auto instruction = cpu.disassembleInstruction()) {
      tracer.instruction->notify(instruction, cpu.disassembleContext());
    }
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
