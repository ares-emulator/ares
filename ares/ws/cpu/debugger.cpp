auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("CPU RAM");
  memory.ram->setSize(SoC::ASWAN() ? 16_KiB : 64_KiB);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return iram.read(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return iram.write(address, data);
  });

  tracer.instruction = parent->append<Node::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(20);

  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "CPU");
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
