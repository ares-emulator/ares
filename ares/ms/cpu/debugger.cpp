auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("CPU RAM");
  memory.ram->setSize(cpu.ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return cpu.ram[address];
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    cpu.ram[address] = data;
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(16);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");

  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(unlikely(tracer.instruction->enabled())) {
    if(tracer.instruction->address(cpu.PC)) {
      tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext());
    }
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(unlikely(tracer.interrupt->enabled())) {
    tracer.interrupt->notify(type);
  }
}

auto CPU::Debugger::in(n16 address, n8 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    string name = "unknown";
    if(0);
    else if((address & 0xff) == 0x00 && Display::LCD()) name = "input port 2";
    else if((address & 0xff) == 0x01 && Display::LCD()) name = "parallel data read";
    else if((address & 0xff) == 0x02 && Display::LCD()) name = "data direction and nmi enable";
    else if((address & 0xff) == 0x03 && Display::LCD()) name = "transmit data buffer";
    else if((address & 0xff) == 0x04 && Display::LCD()) name = "receive data buffer";
    else if((address & 0xff) == 0x05 && Display::LCD()) name = "serial control";
    else if((address & 0xff) == 0x06 && Display::LCD()) name = "psg balance";
    else if((address & 0xc1) == 0x00) name = "bus control";
    else if((address & 0xc1) == 0x01 && Display::CRT() && !Region::NTSCJ()) name = "controller tx read";
    else if((address & 0xc1) == 0x40) name = "vdp vcounter";
    else if((address & 0xc1) == 0x41) name = "vdp hcounter";
    else if((address & 0xc1) == 0x80) name = "vdp data";
    else if((address & 0xc1) == 0x81) name = "vdp status";
    else if((address & 0xff) == 0xf2 && opll.node) name = "opll read";
    else if((address & 0xc1) == 0xc0 && Display::CRT()) name = "input port 0";
    else if((address & 0xc1) == 0xc1 && Display::CRT()) name = "input port 1";
    else if((address & 0xff) == 0xc0 && Display::LCD()) name = "input port 0";
    else if((address & 0xff) == 0xc1 && Display::LCD()) name = "input port 1";
    else if((address & 0xff) == 0xdc && Display::LCD()) name = "input port 0";
    else if((address & 0xff) == 0xdd && Display::LCD()) name = "input port 1";
    tracer.io->notify({
      "$", hex(address, 2L), " = #$", hex(data, 2L),
      " @ ", pad(vdp.vcounter(), 3L), ",", pad(vdp.hcounter(), 3L),
      " ", name
    });
  }
}

auto CPU::Debugger::out(n16 address, n8 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    string name = "unknown";
    if(0);
    else if((address & 0xff) == 0x00 && Display::LCD()) name = "input port 2";
    else if((address & 0xff) == 0x01 && Display::LCD()) name = "parallel data write";
    else if((address & 0xff) == 0x02 && Display::LCD()) name = "data direction and nmi enable";
    else if((address & 0xff) == 0x03 && Display::LCD()) name = "transmit data buffer";
    else if((address & 0xff) == 0x04 && Display::LCD()) name = "receive data buffer";
    else if((address & 0xff) == 0x05 && Display::LCD()) name = "serial control";
    else if((address & 0xff) == 0x06 && Display::LCD()) name = "psg balance";
    else if((address & 0xc1) == 0x00) name = "bus control";
    else if((address & 0xc1) == 0x01 && Display::CRT()) name = "controller tx write";
    else if((address & 0xc0) == 0x40) name = "psg write";
    else if((address & 0xc1) == 0x80) name = "vdp data";
    else if((address & 0xc1) == 0x81) name = "vdp control";
    else if((address & 0xff) == 0xf0 && opll.node) name = "opll address";
    else if((address & 0xff) == 0xf1 && opll.node) name = "opll write";
    else if((address & 0xff) == 0xf2 && opll.node) name = "audio mute";
    tracer.io->notify({
      "$", hex(address, 2L), " = #$", hex(data, 2L),
      " @ ", pad(vdp.vcounter(), 3L), ",", pad(vdp.hcounter(), 3L),
      " ", name
    });
  }
}
