//ROM / RAM access from the S-CPU

auto SuperFX::CPUROM::data() -> uint8* {
  return superfx.rom.data();
}

auto SuperFX::CPUROM::size() const -> uint {
  return superfx.rom.size();
}

auto SuperFX::CPUROM::read(uint24 address, uint8 data) -> uint8 {
  if(superfx.regs.sfr.g && superfx.regs.scmr.ron) {
    static const uint8 vector[16] = {
      0x00, 0x01, 0x00, 0x01, 0x04, 0x01, 0x00, 0x01,
      0x00, 0x01, 0x08, 0x01, 0x00, 0x01, 0x0c, 0x01,
    };
    return vector[address & 15];
  }
  return superfx.rom.read(address, data);
}

auto SuperFX::CPUROM::write(uint24 address, uint8 data) -> void {
  superfx.rom.write(address, data);
}

//

auto SuperFX::CPURAM::data() -> uint8* {
  return superfx.ram.data();
}

auto SuperFX::CPURAM::size() const -> uint {
  return superfx.ram.size();
}

auto SuperFX::CPURAM::read(uint24 address, uint8 data) -> uint8 {
  if(superfx.regs.sfr.g && superfx.regs.scmr.ran) return data;
  return superfx.ram.read(address, data);
}

auto SuperFX::CPURAM::write(uint24 address, uint8 data) -> void {
  superfx.ram.write(address, data);
}

//

auto SuperFX::CPUBRAM::data() -> uint8* {
  return superfx.bram.data();
}

auto SuperFX::CPUBRAM::size() const -> uint {
  return superfx.bram.size();
}

auto SuperFX::CPUBRAM::read(uint24 address, uint8 data) -> uint8 {
  return superfx.bram.read(address, data);
}

auto SuperFX::CPUBRAM::write(uint24 address, uint8 data) -> void {
  if(!superfx.regs.bramr) return;
  superfx.bram.write(address, data);
}
