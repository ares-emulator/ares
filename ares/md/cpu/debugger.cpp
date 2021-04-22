auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("CPU RAM");
  memory.ram->setSize(32_KiB << 1);
  memory.ram->setRead([&](u32 address) -> u8 {
    return cpu.ram[n15(address >> 1)].byte(!(address & 1));
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    cpu.ram[n15(address >> 1)].byte(!(address & 1)) = data;
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.r.pc - 4)) {
    tracer.instruction->notify(cpu.disassembleInstruction(cpu.r.pc - 4), cpu.disassembleContext());
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    string message = {type, " SR=", cpu.r.i, " @ ", vdp.vcounter(), ",", vdp.hcounter()};
    tracer.interrupt->notify(message);
  }
}
