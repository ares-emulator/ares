#define rsp Nintendo64::rsp

auto RSP::Debugger::load(Node::Object parent) -> void {
  memory.dmem = parent->append<Node::Debugger::Memory>("RSP DMEM");
  memory.dmem->setSize(4_KiB);
  memory.dmem->setRead([&](u32 address) -> u8 {
    return rsp.dmem.readByte(address);
  });
  memory.dmem->setWrite([&](u32 address, u8 data) -> void {
    return rsp.dmem.writeByte(address, data);
  });

  memory.imem = parent->append<Node::Debugger::Memory>("RSP IMEM");
  memory.imem->setSize(4_KiB);
  memory.imem->setRead([&](u32 address) -> u8 {
    return rsp.imem.readByte(address);
  });
  memory.imem->setWrite([&](u32 address, u8 data) -> void {
    return rsp.imem.writeByte(address, data);
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "RSP");
  tracer.instruction->setAddressBits(12, 2);

  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "RSP");
}

auto RSP::Debugger::unload() -> void {
  memory.dmem.reset();
  memory.imem.reset();
  tracer.instruction.reset();
  tracer.io.reset();
}

auto RSP::Debugger::instruction() -> void {
  if(tracer.instruction->enabled()) {
    u32 address = rsp.pipeline.address & 0xfff;
    u32 instruction = rsp.pipeline.instruction;
    if(tracer.instruction->address(address)) {
      rsp.disassembler.showColors = 0;
      tracer.instruction->notify(rsp.disassembler.disassemble(address, instruction), {});
      rsp.disassembler.showColors = 1;
    }
  }
}

auto RSP::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}

#undef rsp
