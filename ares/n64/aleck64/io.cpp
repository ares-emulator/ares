auto Aleck64::readWord(u32 address, Thread& thread) -> u32 {
  if(address <= 0xc07f'ffff) {
    return sdram.read<Word>(address);
  }

  controls.poll();
  if(address <= 0xc080'0fff) {
    switch (address) {
      case 0xc080'0000: return readPort1();
      case 0xc080'0002: return readPort1();
      case 0xc080'0004: return readPort2();
      case 0xc080'0006: return readPort2();
    }
  }

  debug(unusual, "[Aleck64::readWord] Unmapped address: 0x", hex(address, 8L));
  return 0xffffffff;
}

auto Aleck64::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if(address <= 0xc07f'ffff) {
    return sdram.write<Word>(address, data);
  }

  if(address <= 0xc080'0fff) {
    switch (address) {

    }
  }

  debug(unusual, "[Aleck64::writeWord] ", hex(address, 8L), " = ", hex(data, 8L));
}

auto Aleck64::readPort1() -> u32 {
  n32 value = 0xffffffff;

  value.bit( 0, 15) = controls.ioPortControls(1);
  value.bit(16, 23) = dipSwitch[1];
  value.bit(24, 31) = dipSwitch[0];

  return value;
}

auto Aleck64::readPort2() -> u32 {
  return controls.ioPortControls(2);
}