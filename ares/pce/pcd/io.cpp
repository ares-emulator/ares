auto PCD::read(uint4 address) -> uint8 {
//print("* ff:180", hex(address, 1L), "\n");

  uint8 data = 0xff;
  return data;
}

auto PCD::write(uint4 address, uint8 data) -> void {
//print("* ff:180", hex(address, 1L), " = ", hex(data, 2L), "\n");
}

auto PCD::readWRAM(uint16 address) -> uint8 {
  return wram.read(address);
}

auto PCD::writeWRAM(uint16 address, uint8 data) -> void {
  return wram.write(address, data);
}

auto PCD::readBRAM(uint11 address) -> uint8 {
  return bram.read(address);
}

auto PCD::writeBRAM(uint11 address, uint8 data) -> void {
  return bram.write(address, data);
}
