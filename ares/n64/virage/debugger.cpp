auto Virage::Debugger::load(Node::Object parent) -> void {
  string name = {"Virage", self.num};
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", name);
  tracer.ioMem = parent->append<Node::Debugger::Tracer::Notification>("I/O MEM", name);

  string sramName = {name, " SRAM"};
  memory.sram = parent->append<Node::Debugger::Memory>(sramName);
  memory.sram->setSize(self.memSize);
  memory.sram->setRead([&](u32 address) -> u8 {
    return self.sram.read<Byte>(address);
  });
  memory.sram->setWrite([&](u32 address, u8 data) -> void {
    return self.sram.write<Byte>(address, data);
  });

  string flashName = {name, " Flash"};
  memory.flash = parent->append<Node::Debugger::Memory>(flashName);
  memory.flash->setSize(self.memSize);
  memory.flash->setRead([&](u32 address) -> u8 {
    return self.flash.read<Byte>(address);
  });
  memory.flash->setWrite([&](u32 address, u8 data) -> void {
    return self.flash.write<Byte>(address, data);
  });
}

auto Virage::Debugger::unload(Node::Object parent) -> void {
  parent->remove(tracer.io);
  parent->remove(tracer.ioMem);
  parent->remove(memory.sram);
  parent->remove(memory.flash);
  tracer.io.reset();
  tracer.ioMem.reset();
  memory.sram.reset();
  memory.flash.reset();
}

auto Virage::Debugger::ioSRAM(bool mode, u32 address, u32 data) -> void {
  if(unlikely(tracer.ioMem->enabled())) {
    string message;
    if(mode == Read) {
      message = {"V", self.num ,"_SRAM[", hex(address, 8L), "] => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", self.num ,"_SRAM[", hex(address, 8L), "] <= ", hex(data, 8L)};
    }
    tracer.ioMem->notify(message);
  }
}

auto Virage::Debugger::ioConfigReg(bool mode, u32 address, u32 data) -> void {
  static const vector<string> registerNames = {
    "REG0",
    "REG1",
    "REG2",
    "REG3",
    "REG4",
    "REG5",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = registerNames(address, "UNKNOWN");
    if(mode == Read) {
      message = {"V", self.num, "_", name, " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", self.num, "_", name, " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto Virage::Debugger::ioStatusReg(bool mode, u32 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    string message;
    if(mode == Read) {
      message = {"V", self.num ,"_STATUS => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", self.num ,"_STATUS <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto Virage::Debugger::ioControlReg(bool mode, u32 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    string message;
    if(mode == Read) {
      message = {"V", self.num ,"_CONTROL => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", self.num ,"_CONTROL <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
