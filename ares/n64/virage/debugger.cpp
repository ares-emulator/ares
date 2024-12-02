auto Virage::Debugger::load(Node::Object parent) -> void {
  string name = {"Virage", num};
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", name);
}

auto Virage::Debugger::ioSRAM(bool mode, u32 address, u32 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    string message;
    if(mode == Read) {
      message = {"V", num ,"_SRAM[", hex(address, 8L), "] => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", num ,"_SRAM[", hex(address, 8L), "] <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
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
      message = {"V", num, "_", name, " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", num, "_", name, " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto Virage::Debugger::ioStatusReg(bool mode, u32 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    string message;
    if(mode == Read) {
      message = {"V", num ,"_STATUS => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", num ,"_STATUS <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto Virage::Debugger::ioControlReg(bool mode, u32 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    string message;
    if(mode == Read) {
      message = {"V", num ,"_CONTROL => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {"V", num ,"_CONTROL <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
