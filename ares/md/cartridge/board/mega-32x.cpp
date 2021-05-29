struct Mega32X : Interface {
  using Interface::Interface;
  Memory::Writable<n16> wram;
  Memory::Writable<n8 > uram;
  Memory::Writable<n8 > lram;
  M24C m24c;

  auto load() -> void override {
    if(auto fp = pak->read("program.rom")) {
      m32x.rom.allocate(fp->size() >> 1);
      for(auto address : range(fp->size() >> 1)) {
        m32x.rom.program(address, fp->readm(2L));
      }
    }

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
    if(address >= 0x200000) {
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
    return m32x.writeExternalIO(upper, lower, address, data);
  }

  auto vblank(bool line) -> void override {
    return m32x.vblank(line);
  }

  auto hblank(bool line) -> void override {
    return m32x.hblank(line);
  }

  auto power(bool reset) -> void override {
    eepromEnable = m32x.rom.size() * 2 <= 0x200000;
    m24c.power();
  }

  auto serialize(serializer& s) -> void override {
    s(wram);
    s(uram);
    s(lram);
    s(m24c);
    s(eepromEnable);
    s(rsda);
    s(wsda);
    s(wscl);
  }

  n1 eepromEnable;
  n4 rsda;
  n4 wsda;
  n4 wscl;
};
