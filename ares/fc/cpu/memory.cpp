//$0000-07ff = RAM (2KB)
//$0800-1fff = RAM (mirror)
//$2000-2007 = PPU
//$2008-3fff = PPU (mirror)
//$4000-4017 = APU + I/O
//$4020-403f = FDS
//$4018-ffff = Cartridge

inline auto CPU::readBus(n16 address) -> n8 {
  n8 data = cartridge.readPRG(address, MDR);
  if(address <= 0x1fff) return ram.read(address);
  if(address <= 0x3fff) return ppu.readIO(address);
  if(address <= 0x4017) return cpu.readIO(address);
  return data;
}

inline auto CPU::writeBus(n16 address, n8 data) -> void {
  cartridge.writePRG(address, data);
  if(address <= 0x1fff) return ram.write(address, data);
  if(address <= 0x3fff) return ppu.writeIO(address, data);
  if(address <= 0x4017) return cpu.writeIO(address, data);
}

auto CPU::readIO(n16 address) -> n8 {
  n8 data = MDR;

  switch(address) {

  case 0x4016: {
    auto port1 = controllerPort1.data();
    auto port3 = expansionPort.read1();
    platform->input(system.controls.microphone);
    data.bit(0) = port1.bit(0);
    data.bit(1) = port3.bit(0);
    data.bit(2) = system.controls.microphone->value() ? random().bit(0) : 0;
    data.bit(3) = port1.bit(1);
    data.bit(4) = port1.bit(2);
    return data;
  }

  case 0x4017: {
    auto port2 = controllerPort2.data();
    auto port3 = expansionPort.read2();
    data.bit(0) = port3.bit(0) | port2.bit(0);
    data.bit(1) = port3.bit(1);
    data.bit(2) = port3.bit(2);
    data.bit(3) = port3.bit(3) | port2.bit(1);
    data.bit(4) = port3.bit(4) | port2.bit(2);
    return data;
  }

  }

  return apu.readIO(address);
}

auto CPU::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x4014: {
    io.oamDMAPage = data;
    io.oamDMAPending = 1;
    return;
  }

  case 0x4016: {
    controllerPort1.latch(data.bit(0));
    controllerPort2.latch(data.bit(0));
    expansionPort.write(data.bit(0,2));
    return;
  }

  }

  return apu.writeIO(address, data);
}

auto CPU::readDebugger(n16 address) -> n8 {
  return readBus(address);
}
