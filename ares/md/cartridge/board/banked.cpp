struct Banked : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;
  Memory::Writable<n16> wram;
  Memory::Writable<n8 > uram;
  Memory::Writable<n8 > lram;
  M24C m24c;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    if(auto fp = pak->read("save.ram")) {
      Interface::load(wram, uram, lram, "save.ram");
    }
    if(auto fp = pak->read("save.eeprom")) {
      Interface::load(m24c, "save.eeprom");
      rsda = fp->attribute("rsda").natural();
      wsda = fp->attribute("wsda").natural();
      wscl = fp->attribute("wscl").natural();
    }
  }

  auto save() -> void override {
    if(auto fp = pak->write("save.ram")) {
      Interface::save(wram, uram, lram, "save.ram");
    }
    if(auto fp = pak->write("save.eeprom")) {
      Interface::save(m24c, "save.eeprom");
    }
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    if(address >= 0x200000) {
      if(wram && ramEnable) {
        return wram[address >> 1];
      }

      if(uram && ramEnable) {
        return uram[address >> 1] * 0x0101;
      }

      if(lram && ramEnable) {
        return lram[address >> 1] * 0x0101;
      }

      if(m24c && eepromEnable) {
        if(upper && rsda >> 3 == 1) data.bit(rsda) = m24c.read();
        if(lower && rsda >> 3 == 0) data.bit(rsda) = m24c.read();
        return data;
      }
    }

    n25 offset = romBank[address >> 19] << 19 | (n19)address;

    if ((offset >> 1) > rom.size() - 1) {
      return 0xffff;
    }

    return data = rom[offset >> 1];
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
   //emulating ramWritable will break commercial software:
    //it does not appear that many (any?) games actually connect $a130f1.d1 to /WE;
    //hence RAM ends up always being writable, and many games fail to set d1=1
    if(address >= 0x200000) {
      if(wram && ramEnable) {
        if(upper) wram[address >> 1].byte(1) = data.byte(1);
        if(lower) wram[address >> 1].byte(0) = data.byte(0);
        return;
      }

      if(uram && ramEnable) {
        if(upper) uram[address >> 1] = data;
        return;
      }

      if(lram && ramEnable) {
        if(lower) lram[address >> 1] = data;
        return;
      }

      if(m24c) {
        if(rom.size() * 2 > 0x200000 && upper && lower) {
          eepromEnable = !data.bit(0);
          return;
        }
        if(eepromEnable) {
          if(upper && wscl >> 3 == 1) m24c.clock = data.bit(wscl);
          if(upper && wsda >> 3 == 1) m24c.data  = data.bit(wsda);
          if(lower && wscl >> 3 == 0) m24c.clock = data.bit(wscl);
          if(lower && wsda >> 3 == 0) m24c.data  = data.bit(wsda);
          return m24c.write();
        }
      }
    }
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    return data;
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(!lower) {
      //todo: unconfirmed
      debug(unusual, "[Cartridge::Banked::writeIO] lower=0");
      return;
    }

    if(address == 0xa130f0) {
      ramEnable   = data.bit(0);
      ramWritable = data.bit(1);
    }

    if(address == 0xa130f2) romBank[1] = data.bit(0,5);
    if(address == 0xa130f4) romBank[2] = data.bit(0,5);
    if(address == 0xa130f6) romBank[3] = data.bit(0,5);
    if(address == 0xa130f8) romBank[4] = data.bit(0,5);
    if(address == 0xa130fa) romBank[5] = data.bit(0,5);
    if(address == 0xa130fc) romBank[6] = data.bit(0,5);
    if(address == 0xa130fe) romBank[7] = data.bit(0,5);
  }

  auto power(bool reset) -> void override {
    for(auto index : range(8)) romBank[index] = index;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
    s(wram);
    s(uram);
    s(lram);
    s(m24c);
    s(ramEnable);
    s(ramWritable);
    s(eepromEnable);
    s(rsda);
    s(wsda);
    s(wscl);
  }

  n6 romBank[8];
  n1 ramEnable;
  n1 ramWritable;
  n1 eepromEnable;
  n4 rsda;
  n4 wsda;
  n4 wscl;
};
