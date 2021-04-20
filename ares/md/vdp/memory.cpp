auto VDP::VRAM::read(n16 address) const -> n16 {
  if(mode == 0) {
    return memory[(n15)address];
  } else {
    n15 offset = address >> 1 & 0x7e00 | address & 0x01fe | address >> 9 & 1;
    n8  data = memory[offset].byte(address.bit(0));
    return data << 8 | data << 0;
  }
}

auto VDP::VRAM::write(n16 address, n16 data) -> void {
  if(mode == 0) {
    memory[(n15)address] = data;
  } else {
    n15 offset = address >> 1 & 0x7e00 | address & 0x01fe | address >> 9 & 1;
    memory[offset].byte(address.bit(0)) = data.byte(0);
  }
  vdp.sprite.write(address, data);
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

auto VDP::CRAM::color(n6 address) const -> n9 {
  return memory[address];
}

auto VDP::CRAM::read(n6 address) const -> n16 {
  n16 data = memory[address];
  return data.bit(0,2) << 1 | data.bit(3,5) << 5 | data.bit(6,8) << 9;
}

auto VDP::CRAM::write(n6 address, n16 data) -> void {
  data = data.bit(1,3) << 0 | data.bit(5,7) << 3 | data.bit(9,11) << 6;
  memory[address] = data;
}
