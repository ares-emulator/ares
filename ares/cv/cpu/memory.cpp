auto CPU::read(uint16 address) -> uint8 {
  uint8 data = 0xff;
  if(address >= 0x0000 && address <= 0x1fff && io.replaceBIOS) return expansion.read(address);
  if(address >= 0x2000 && address <= 0x7fff && io.replaceRAM ) return expansion.read(address);
  if(address >= 0x0000 && address <= 0x1fff) return system.bios[address & 0x1fff];
  if(address >= 0x6000 && address <= 0x7fff) return ram.read(address - 0x6000);
  if(address >= 0x8000 && address <= 0xffff) return cartridge.rom.read(address - 0x8000);
  return data;
}

auto CPU::write(uint16 address, uint8 data) -> void {
  if(address >= 0x0000 && address <= 0x1fff && io.replaceBIOS) return expansion.write(address, data);
  if(address >= 0x2000 && address <= 0x7fff && io.replaceRAM)  return expansion.write(address, data);
  if(address >= 0x6000 && address <= 0x7fff) return ram.write(address - 0x6000, data);
  if(address >= 0x8000 && address <= 0xffff) return;
}

auto CPU::in(uint16 address) -> uint8 {
  address &= 0xff;
  uint8 data = 0xff;
  if(address >= 0xa0 && address <= 0xbf) return !address.bit(0) ? vdp.data() : vdp.status();
  if(address >= 0xe0 && address <= 0xff && address.bit(1) == 0) return controllerPort1.read();
  if(address >= 0xe0 && address <= 0xff && address.bit(1) == 1) return controllerPort2.read();
  return data;
}

auto CPU::out(uint16 address, uint8 data) -> void {
  address &= 0xff;
  if(address == 0x53) io.replaceRAM  = data.bit(0);
  if(address == 0x7f) io.replaceBIOS = data.bit(1);
  if(address >= 0x80 && address <= 0x9f) controllerPort1.write(0), controllerPort2.write(0);
  if(address >= 0xa0 && address <= 0xbf) return !address.bit(0) ? vdp.data(data) : vdp.control(data);
  if(address >= 0xc0 && address <= 0xdf) controllerPort1.write(1), controllerPort2.write(1);
  if(address >= 0xe0 && address <= 0xff) return psg.write(data);
}
