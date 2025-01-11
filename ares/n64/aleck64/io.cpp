auto Aleck64::readWord(u32 address, Thread& thread) -> u32 {
  if(address <= 0xc07f'ffff) {
    return sdram.read<Word>(address & 0x00ff'ffff);
  }

  controls.poll();
  if(address <= 0xc080'0fff) {
    switch (address & 0xffff'fffc) {
      case 0xc080'0000: return readPort1();
      case 0xc080'0004: return readPort2();
      case 0xc080'0008: return readPort3();
      case 0xc080'0100: return readPort4();
    }
  }

  debug(unusual, "[Aleck64::readWord] Unmapped address: 0x", hex(address, 8L));
  return 0xffffffff;
}

auto Aleck64::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if(address <= 0xc07f'ffff) {
    return sdram.write<Word>(address & 0x00ff'ffff, data);
  }

  if(address <= 0xc080'0fff) {
    switch (address & 0xffff'fffc) {
      case 0xc080'0008: return writePort3(data);
      case 0xc080'0100: return writePort4(data);
    }
  }

  print("[Aleck64::writeWord] Unmapped address: 0x", hex(address, 8L), " = 0x", hex(data, 8L), "\n");
  //debug(unusual, "[Aleck64::writeWord] ", hex(address, 8L), " = ", hex(data, 8L));
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

auto Aleck64::readPort3() -> u32 {
  if(gameConfig) return gameConfig->readExpansionPort();
  //debug(unusual, "[Aleck64::readPort3]");
  print("[Aleck64::readPort3]\n");
  return 0xffff'ffff;
}

auto Aleck64::writePort3(n32 data) -> void {
  if(gameConfig) return gameConfig->writeExpansionPort(data);
  //debug(unusual, "[Aleck64::writePort3] ", hex(data, 8L));
  print("[Aleck64::writePort3] ", hex(data, 8L), "\n");
}

auto Aleck64::readPort4() -> u32 {
  //debug(unimplemented, "[Aleck64::readPort4]");
  print("[Aleck64::readPort4]\n");
  return 0x0;
}

auto Aleck64::writePort4(n32 data) -> void {
  // debug(unimplemented, "[Aleck64::writePort3] ", hex(data, 8L));
  print("[Aleck64::writePort4] ", hex(data, 8L), "\n");
}
