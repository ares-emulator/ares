auto CPU::read(n16 address) -> n8 {

  // VIDEO
  if (address == 0xe000) return vdp.data();
  if (address == 0xe002) return vdp.status();

  // MAIN
  if (address < 0x6000) return cartridge.read(address);
  if (address >= 0xa000 && address <= 0xa800) return ram.read(address - 0xa000);

  return 0;
}

auto CPU::write(n16 address, n8 data) -> void {

  // VIDEO
  if (address == 0xe000) return vdp.data(data);
  if (address == 0xe002) return vdp.control(data);

  // MAIN
  if (address < 0x6000) return; // ALL RETAIL GAMES ARE WRITE ONLY
  if (address >= 0xa000 && address <= 0xffff) return ram.write(address - 0xa000, data);

  return;
}

auto CPU::out(n16 address, n8 data) -> void {
  address &= 0xff;
  if (address == 0x00) {
    return psg.select(data);
  }
  if (address == 0x01) {
    return psg.write(data);
  }
}

auto CPU::in(n16 address) -> n8 {
  address &= 0xff;
  if (address == 0x02) {
	  return psg.read();
  }

  return 0xff;
}
