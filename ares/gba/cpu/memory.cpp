auto CPU::readIWRAM(u32 mode, n32 address) -> n32 {
  if(mode & Word) return readIWRAM(Half, address &~ 2) << 0 | readIWRAM(Half, address | 2) << 16;
  if(mode & Half) return readIWRAM(Byte, address &~ 1) << 0 | readIWRAM(Byte, address | 1) <<  8;

  return iwram[address & 0x7fff];
}

auto CPU::writeIWRAM(u32 mode, n32 address, n32 word) -> void {
  if(mode & Word) {
    writeIWRAM(Half, address &~2, word >>  0);
    writeIWRAM(Half, address | 2, word >> 16);
    return;
  }

  if(mode & Half) {
    writeIWRAM(Byte, address &~1, word >>  0);
    writeIWRAM(Byte, address | 1, word >>  8);
    return;
  }

  iwram[address & 0x7fff] = word;
}

auto CPU::readEWRAM(u32 mode, n32 address) -> n32 {
  if(!memory.ewram) return readIWRAM(mode, address);

  if(mode & Word) return readEWRAM(Half, address &~ 2) << 0 | readEWRAM(Half, address | 2) << 16;
  if(mode & Half) return readEWRAM(Byte, address &~ 1) << 0 | readEWRAM(Byte, address | 1) <<  8;

  return ewram[address & 0x3ffff];
}

auto CPU::writeEWRAM(u32 mode, n32 address, n32 word) -> void {
  if(!memory.ewram) return writeIWRAM(mode, address, word);

  if(mode & Word) {
    writeEWRAM(Half, address &~2, word >>  0);
    writeEWRAM(Half, address | 2, word >> 16);
    return;
  }

  if(mode & Half) {
    writeEWRAM(Byte, address &~1, word >>  0);
    writeEWRAM(Byte, address | 1, word >>  8);
    return;
  }

  ewram[address & 0x3ffff] = word;
}
