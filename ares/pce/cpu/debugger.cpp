auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("CPU RAM");
  memory.ram->setSize(cpu.ram.size());
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return cpu.ram.read(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return cpu.ram.write(address, data);
  });

  tracer.instruction = parent->append<Node::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled()) {
    auto bank = cpu.r.mpr[cpu.r.pc >> 13];
    auto address = (uint13)cpu.r.pc;
    if(tracer.instruction->address(bank << 16 | address)) {
      tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext(), {
        "V:", pad(vdp.io.vcounter, 3L), " ", "H:", pad(vdp.io.hcounter, 4L)
      });
    }
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
