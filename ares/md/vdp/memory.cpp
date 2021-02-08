auto VDP::VRAM::read(n16 address) const -> n16 {
  if(mode == 0) {
    return memory[(n15)address];
  } else {
    n15 offset = address >> 1 & 0x7e00 | address & 0x01fe | address >> 9 & 1;
    n8  data = memory[offset].byte(!address.bit(0));
    return data << 8 | data << 0;
  }
}

auto VDP::VRAM::write(n16 address, n16 data) -> void {
  if(mode == 0) {
    memory[(n15)address] = data;
  } else {
    n15 offset = address >> 1 & 0x7e00 | address & 0x01fe | address >> 9 & 1;
    memory[offset].byte(!address.bit(0)) = data.byte(0);
  }
  if(address < vdp.sprite.io.nametableAddress) return;
  if(address > vdp.sprite.io.nametableAddress + 319) return;
  vdp.sprite.write(address - vdp.sprite.io.nametableAddress, data);
}

auto VDP::VRAM::readByte(n17 address) const -> n8 {
  return read(address >> 1).byte(!address.bit(0));
}

auto VDP::VRAM::writeByte(n17 address, n8 data) -> void {
  auto word = read(address >> 1);
  word.byte(!address.bit(0)) = data;
  write(address >> 1, word);
}

auto VDP::VSRAM::read(n6 address) const -> n10 {
  if(address >= 40) return 0x0000;
  return memory[address];
}

auto VDP::VSRAM::write(n6 address, n10 data) -> void {
  if(address >= 40) return;
  memory[address] = data;
}

auto VDP::CRAM::read(n6 address) const -> n9 {
  return memory[address];
}

auto VDP::CRAM::write(n6 address, n9 data) -> void {
  memory[address] = data;
}
