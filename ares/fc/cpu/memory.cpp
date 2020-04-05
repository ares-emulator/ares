auto CPU::readRAM(uint11 address) -> uint8 {
  return ram[address];
}

auto CPU::writeRAM(uint11 address, uint8 data) -> void {
  ram[address] = data;
}

auto CPU::readIO(uint16 address) -> uint8 {
  uint8 data = mdr();

  switch(address) {

  case 0x4016: {
    auto poll = controllerPort1.data();
    platform->input(system.controls.microphone);
    data.bit(0) = poll.bit(0);
    data.bit(1) = 0;
    data.bit(2) = system.controls.microphone->value() ? random().bit(0) : 0;
    data.bit(3) = poll.bit(1);
    data.bit(4) = poll.bit(2);
    return data;
  }

  case 0x4017: {
    auto poll = controllerPort2.data();
    data.bit(0) = poll.bit(0);
    data.bit(1) = 0;
    data.bit(2) = 0;
    data.bit(3) = poll.bit(1);
    data.bit(4) = poll.bit(2);
    return data;
  }

  }

  return apu.readIO(address);
}

auto CPU::writeIO(uint16 address, uint8 data) -> void {
  switch(address) {

  case 0x4014: {
    io.oamDMAPage = data;
    io.oamDMAPending = 1;
    return;
  }

  case 0x4016: {
    controllerPort1.latch(data.bit(0));
    controllerPort2.latch(data.bit(0));
    return;
  }

  }

  return apu.writeIO(address, data);
}

auto CPU::readDebugger(uint16 address) -> uint8 {
  return bus.read(address);
}
