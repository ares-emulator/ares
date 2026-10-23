auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("CPU RAM");
  memory.ram->setSize(self.ram.size);
  memory.ram->setRead([&](u32 address) -> u8 {
    return self.ram.readByte(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return self.ram.writeByte(address, data);
  });

  memory.scratchpad = parent->append<Node::Debugger::Memory>("CPU Scratchpad");
  memory.scratchpad->setSize(self.scratchpad.size);
  memory.scratchpad->setRead([&](u32 address) -> u8 {
    return self.scratchpad.readByte(address);
  });
  memory.scratchpad->setWrite([&](u32 address, u8 data) -> void {
    return self.scratchpad.writeByte(address, data);
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(32, 2);
  tracer.instruction->setDepth(32);

  tracer.exception = parent->append<Node::Debugger::Tracer::Notification>("Exception", "CPU");
  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
  tracer.message = parent->append<Node::Debugger::Tracer::Notification>("Message", "CPU");
  tracer.function = parent->append<Node::Debugger::Tracer::Notification>("Function", "CPU");

  tracer.message->setAutoLineBreak(false);
  tracer.message->setTerminal(true);
}

auto CPU::Debugger::unload() -> void {
  memory.ram.reset();
  memory.scratchpad.reset();
  tracer.instruction.reset();
  tracer.exception.reset();
  tracer.interrupt.reset();
  tracer.message.reset();
  tracer.function.reset();
}

auto CPU::Debugger::instruction() -> void {
  if(!tracer.instruction->enabled()) return;

  u32 address = self.pipeline.address;
  u32 instruction = self.pipeline.instruction;
  if(tracer.instruction->address(address)) {
    disassembler.showColors = 0;
    tracer.instruction->notify(disassembler.disassemble(address, instruction), {});
    disassembler.showColors = 1;
  }
}

auto CPU::Debugger::exception(u8 code) -> void {
  if(!tracer.exception->enabled()) return;

  string type;
  if(code ==  0) type = "Interrupt";
  if(code ==  4) type = "AddressLoad";
  if(code ==  5) type = "AddressStore";
  if(code ==  6) type = "BusInstruction";
  if(code ==  7) type = "BusData";
  if(code ==  8) type = "SystemCall";
  if(code ==  9) type = "Breakpoint";
  if(code == 10) type = "ReservedInstruction";
  if(code == 11) type = "CoprocessorDisabled";
  if(code == 12) type = "ArithmeticOverflow";
  if(code == 13) type = "Trap";

  if(code ==  0) return;  //interrupt exceptions are logged by interrupt() instead
  if(code ==  8) return;  //ignore SYSCALL exceptions (they are used often to call BIOS functions)

  tracer.exception->notify({type, " PC:", hex(self.ipu.pc, 8L)});
}

auto CPU::Debugger::interrupt(u8 mask) -> void {
  if(!tracer.interrupt->enabled()) return;

  string source;
  if(mask & 0x01) {
    source.append("Software0,");
  }
  if(mask & 0x02) {
    source.append("Software1,");
  }
  if(mask & 0x04) {
    if(PlayStation::interrupt.vblank.poll()) source.append("Vblank,");
    if(PlayStation::interrupt.gpu.poll()) source.append("GPU,");
    if(PlayStation::interrupt.cdrom.poll()) source.append("CDROM,");
    if(PlayStation::interrupt.dma.poll()) source.append("DMA,");
    if(PlayStation::interrupt.timer0.poll()) source.append("Timer0,");
    if(PlayStation::interrupt.timer1.poll()) source.append("Timer1,");
    if(PlayStation::interrupt.timer2.poll()) source.append("Timer2,");
    if(PlayStation::interrupt.peripheral.poll()) source.append("Peripheral,");
    if(PlayStation::interrupt.sio.poll()) source.append("SIO,");
    if(PlayStation::interrupt.spu.poll()) source.append("SPU,");
    if(PlayStation::interrupt.pio.poll()) source.append("PIO,");
  }
  source.trimRight(",", 1L);
  tracer.interrupt->notify(source);
}
