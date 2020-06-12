#define rsp Nintendo64::rsp

auto RSP::Debugger::load(Node::Object parent) -> void {
  tracer.instruction = parent->append<Node::Instruction>("Instruction", "RSP");
  tracer.instruction->setAddressBits(12);

  tracer.io = parent->append<Node::Notification>("I/O", "RSP");

  memory.dmem = parent->append<Node::Memory>("RSP DMEM");
  memory.dmem->setSize(4_KiB);
  memory.dmem->setRead([&](uint32 address) -> uint8 {
    return rsp.dmem.readByte(address);
  });
  memory.dmem->setWrite([&](uint32 address, uint8 data) -> void {
    return rsp.dmem.writeByte(address, data);
  });

  memory.imem = parent->append<Node::Memory>("RSP IMEM");
  memory.imem->setSize(4_KiB);
  memory.imem->setRead([&](uint32 address) -> uint8 {
    return rsp.imem.readByte(address);
  });
  memory.imem->setWrite([&](uint32 address, uint8 data) -> void {
    return rsp.imem.writeByte(address, data);
  });
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
