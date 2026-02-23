// This implements the 4Mbit MBM29DL400TC NOR flash chip.
// Not implemented:
// - sector protection
// - program/erase operation timings (they're instant)
// - sector banks (it's respected for sector erase, but nowhere else)

auto Cartridge::FLASH::read(n19 address, bool words) -> n16 {
  // TODO: Is this correct?
  if(fastmode) {
    // Embedded Program status - completed
    n8 data = 0;
    data.bit(7) = self.rom.read(address).bit(7);
    data.bit(2) = 1;
    return data;
  }

  if(idmode) {
    if(!words) {
      if(address & 1) return 0;
      address >>= 1;
    }
    if((address & 0xFFF) == 0x000) return 0x04;
    if((address & 0xFFF) == 0x001) return 0x220C;
    return 0;
  }

  if (!words) return self.rom.read(address);
  return self.rom.read(address << 1) | (self.rom.read((address << 1) | 1) << 8);
}

auto Cartridge::FLASH::write(n19 address, n8 byte) -> void {
  if(programmode) {
    self.rom.write(address, self.rom.read(address) & byte);
    programmode = false;
    return;
  }

  if(fastmode) {
    if(byte == 0xa0) programmode = true;
    if(byte == 0x90) fastmode = false;
    return;  
  }

  if(unlock == 0) {
    if(byte == 0xf0) idmode = false;
    if(byte == 0xaa && (address & 0xfff) == 0xaaa) unlock = 1;
  } else if(unlock == 1) {
    if(byte == 0x55 && (address & 0xfff) == 0x555) unlock = 2;
    else unlock = 0;
  } else if(unlock == 2) {
    if(erasemode) {
      if(byte == 0x10) {
        for(u32 n : range(self.rom.size())) self.rom.write(n, 0xff);
      }
      if(byte == 0x30) {
        u32 offset, size;
        if(address < 0x60000)      { offset = address & 0x70000; size = 0x10000; }
        else if(address < 0x64000) { offset = 0x60000;           size = 0x4000;  }
        else if(address < 0x6c000) { offset = 0x64000;           size = 0x8000;  }
        else if(address < 0x74000) { offset = address & 0x7e000; size = 0x2000;  }
        else if(address < 0x7c000) { offset = 0x74000;           size = 0x8000;  }
        else                       { offset = 0x7c000;           size = 0x4000;  }
        for(u32 n : range(size)) self.rom.write(offset++, 0xff);
      }
      erasemode = false;
    }

    if(byte == 0xf0) idmode = false;
    if(byte == 0x90) idmode = true;
    if(byte == 0xa0) programmode = true;
    if(byte == 0x80) erasemode = true;
    if(byte == 0x20) fastmode = true;

    unlock = 0;
  }
}

auto Cartridge::FLASH::power() -> void {
  unlock = 0;
  idmode = false;
  programmode = false;
  fastmode = false;
  erasemode = false;
}

