auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("CPU RAM");
  memory.ram->setSize(SoC::ASWAN() ? 16_KiB : 64_KiB);
  memory.ram->setRead([&](u32 address) -> u8 {
    return iram.read(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return iram.write(address, data);
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(20);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
}

auto CPU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.ram);
  parent->remove(tracer.instruction);
  parent->remove(tracer.interrupt);
  memory.ram.reset();
  tracer.instruction.reset();
  tracer.interrupt.reset();
}

auto CPU::Debugger::ports() -> string {
  string output;

  static const string irqName[8] = {
    "SerialSend",
    "Input",
    "Cartridge",
    "SerialReceive",
    "LineCompare",
    "VblankTimer",
    "Vblank",
    "HblankTimer"
  };

  output.append("SoC Interrupt Base: ", hex(self.io.interruptBase, 2), "\n");

  output.append("SoC Interrupts Enabled: ");
  u32 iAdded = 0;
  for(u32 i : range(8)) {
    if(self.io.interruptEnable & (1 << i)) {
      if(iAdded++ > 0) output.append(", ");
      output.append(irqName[i]);
    }
  }
  output.append("\n");

  output.append("SoC Interrupts Raised: ");
  iAdded = 0;
  for(u32 i : range(8)) {
    if(self.io.interruptStatus & (1 << i)) {
      if(iAdded++ > 0) output.append(", ");
      output.append(irqName[i]);
    }
  }
  output.append("\n");

  output.append("NMI on Low Battery: ", self.io.nmiOnLowBattery ? "enabled" : "disabled", "\n");

  output.append("Cartridge Bus: ",
    self.io.cartridgeEnable ? "enabled" : "disabled", ", ",
    self.io.cartridgeRomWidth ? "16-bit" : "8-bit", ", ",
    self.io.cartridgeRomWait ? "3 wait cycles" : "1 wait cycle",
    "\n");

  return output;
}

auto CPU::Debugger::instruction() -> void {
  if(likely(!tracer.instruction->enabled())) return;
  if(tracer.instruction->address(n20(self.PS * 16 + self.PC))) {
    if(auto instruction = self.disassembleInstruction()) {
      tracer.instruction->notify(instruction, {self.disassembleContext(), " V:", pad(ppu.vcounter(), 3L), " H:", pad(ppu.hcounter(), 3L)});
    }
  }
}

auto CPU::Debugger::interrupt(n3 type) -> void {
  if(likely(!tracer.interrupt->enabled())) return;
  static const string name[8] = {
    "SerialSend",
    "Input",
    "Cartridge",
    "SerialReceive",
    "LineCompare",
    "VblankTimer",
    "Vblank",
    "HblankTimer"
  };
  tracer.interrupt->notify(name[type]);
}
