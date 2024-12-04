auto MI::Debugger::load(Node::Object parent) -> void {
  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "RCP");
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "MI");

  memory.rom = parent->append<Node::Debugger::Memory>("Boot ROM");
  memory.rom->setSize(0x2000);
  memory.rom->setRead([&](u32 address) -> u8 {
    return mi.rom.read<Byte>(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return;
  });

  memory.ram = parent->append<Node::Debugger::Memory>("SK RAM");
  memory.ram->setSize(0x10000);
  memory.ram->setRead([&](u32 address) -> u8 {
    return mi.ram.read<Byte>(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return mi.ram.write<Byte>(address, data);
  });

  memory.scratch = parent->append<Node::Debugger::Memory>("Secure Scratch");
  memory.scratch->setSize(0x8000);
  memory.scratch->setRead([&](u32 address) -> u8 {
    return mi.scratch.read<Byte>(address);
  });
  memory.scratch->setWrite([&](u32 address, u8 data) -> void {
    return mi.scratch.write<Byte>(address, data);
  });
}

auto MI::Debugger::interrupt(u8 source) -> void {
  if(unlikely(tracer.interrupt->enabled())) {
    string type = "unknown";
    if(source == (u32)MI::IRQ::SP)     type = "SP";
    if(source == (u32)MI::IRQ::SI)     type = "SI";
    if(source == (u32)MI::IRQ::AI)     type = "AI";
    if(source == (u32)MI::IRQ::VI)     type = "VI";
    if(source == (u32)MI::IRQ::PI)     type = "PI";
    if(source == (u32)MI::IRQ::DP)     type = "DP";
    if(source == (u32)MI::IRQ::FLASH)  type = "Flash";
    if(source == (u32)MI::IRQ::AES)    type = "AES";
    if(source == (u32)MI::IRQ::IDE)    type = "IDE";
    if(source == (u32)MI::IRQ::PI_ERR) type = "PI error";
    if(source == (u32)MI::IRQ::USB0)   type = "USB0";
    if(source == (u32)MI::IRQ::USB1)   type = "USB1";
    if(source == (u32)MI::IRQ::BTN)    type = "Button";
    if(source == (u32)MI::IRQ::MD)     type = "Module";
    tracer.interrupt->notify(type);
  }
}

auto MI::Debugger::io(bool mode, u32 address, u32 data) -> void {
  static const vector<string> registerNames = {
    "MI_INIT_MODE",
    "MI_VERSION",
    "MI_INTR",
    "MI_INTR_MASK",
    "MI_UNKNOWN",
    "MI_BB_SECURE_EXCEPTION"
    "MI_UNKNOWN",
    "MI_UNKNOWN",
    "MI_UNKNOWN",
    "MI_UNKNOWN",
    "MI_UNKNOWN",
    "MI_BB_RANDOM"
    "MI_UNKNOWN",
    "MI_UNKNOWN",
    "MI_BB_INTERRUPT"
    "MI_BB_MASK"
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = registerNames(address, "MI_UNKNOWN");
    if(mode == Read) {
      message = {name.split("|").first(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {name.split("|").last(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto MI::Debugger::ioMem(bool mode, u32 address, u32 data, const char *name) -> void {
  if(unlikely(tracer.io->enabled())) {
    string message;
    if(mode == Read) {
      message = {name, "[", hex(address, 8L), "]", " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {name, "[", hex(address, 8L), "]", " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
