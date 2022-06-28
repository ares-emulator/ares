struct Mega32XBanked : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;
  Memory::Writable<n16> wram;
  Memory::Writable<n8 > uram;
  Memory::Writable<n8 > lram;
  u32 sramAddr, sramSize;
  M24C m24c;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    if(auto fp = pak->read("program.rom")) {
      m32x.rom.allocate(fp->size() >> 1);
      for(auto address : range(fp->size() >> 1)) {
        m32x.rom.program(address, fp->readm(2L));
      }
    }

    if(auto fp = pak->read("save.ram")) {
      Interface::load(sramAddr, sramSize, wram, uram, lram, "save.ram");
    }

    if(auto fp = pak->read("save.eeprom")) {
      Interface::load(sramAddr, sramSize, m24c, "save.eeprom");
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
    if(address >= sramAddr && address < sramAddr+sramSize) {
      if(wram) {
        return wram[address >> 1];
      }

      if(uram) {
        return uram[address >> 1] * 0x0101;
      }

      if(lram) {
        return lram[address >> 1] * 0x0101;
      }

      if(m24c && eepromEnable) {
        if(upper && rsda >> 3 == 1) data.bit(rsda) = m24c.read();
        if(lower && rsda >> 3 == 0) data.bit(rsda) = m24c.read();
        return data;
      }
    }

    return m32x.readExternal(upper, lower, address, data);
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
    if(address >= sramAddr && address < sramAddr+sramSize) {
      if(wram) {
        if(upper) wram[address >> 1].byte(1) = data.byte(1);
        if(lower) wram[address >> 1].byte(0) = data.byte(0);
        return;
      }

      if(uram) {
        if(upper) uram[address >> 1] = data;
        return;
      }

      if(lram) {
        if(lower) lram[address >> 1] = data;
        return;
      }

      if(m24c) {
        if(m32x.rom.size() * 2 > 0x200000 && upper && lower) {
          eepromEnable = !data.bit(0);
          return;
        }
        if(upper && wscl >> 3 == 1) m24c.clock = data.bit(wscl);
        if(upper && wsda >> 3 == 1) m24c.data  = data.bit(wsda);
        if(lower && wscl >> 3 == 0) m24c.clock = data.bit(wscl);
        if(lower && wsda >> 3 == 0) m24c.data  = data.bit(wsda);
        return m24c.write();
      }
    }

    return m32x.writeExternal(upper, lower, address, data);
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    return m32x.readExternalIO(upper, lower, address, data);
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if (address >= 0xa130f2 && address <= 0xa130fe) {
      if(!lower) {
        //todo: unconfirmed
        debug(unusual, "[Cartridge::Banked::writeIO] lower=0");
        return;
      }
      
      // todo: confirm behavior of accessing with RV clear
      auto bankSet = ((address & 0xf) >> 1) - 1;
      romBank[bankSet] = data.bit(0,5);
      
      auto romAddr = romBank[bankSet] << 18;
      auto bankAddr = (bankSet + 1) << 18;
      for(auto offset : range(1 << 18)) {
        m32x.rom.program(bankAddr + offset, rom[romAddr + offset]);
      }
      
      return;
    }
    
    return m32x.writeExternalIO(upper, lower, address, data);
  }

  auto vblank(bool line) -> void override {
    return m32x.vblank(line);
  }

  auto hblank(bool line) -> void override {
    return m32x.hblank(line);
  }

  auto power(bool reset) -> void override {
    for(auto index : range(8)) romBank[index] = index;
    eepromEnable = m32x.rom.size() * 2 <= 0x200000;
    m24c.power();
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
    s(wram);
    s(uram);
    s(lram);
    s(m24c);
    s(eepromEnable);
    s(rsda);
    s(wsda);
    s(wscl);
  }

  n6 romBank[8];
  n1 eepromEnable;
  n4 rsda;
  n4 wsda;
  n4 wscl;
};
