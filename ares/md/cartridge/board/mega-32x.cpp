struct Mega32X : Interface {
  using Interface::Interface;
  Memory::Writable<n16> wram;
  Memory::Writable<n8 > bram;
  M24Cxx m24cxx;
  enum class Storage : u32 {
    None,
    WordRAM,
    UpperRAM,
    LowerRAM,
    M28C16,
    M24C01,
    M24C02,
    M24C04,
    M24C08,
    M24C16,
  };

  auto load() -> void override {
    storage = Storage::None;
    m24cxx.erase();

    if(auto fp = pak->read("program.rom")) {
      m32x.rom.allocate(fp->size() >> 1);
      for(auto address : range(fp->size() >> 1)) {
        m32x.rom.program(address, fp->readm(2L));
      }
    }

    if(auto fp = pak->read("save.ram")) {
      auto mode = fp->attribute("mode");
      if(mode == "word") {
        storage = Storage::WordRAM;
        Interface::load(wram, "save.ram");
      }
      if(mode == "upper") {
        storage = Storage::UpperRAM;
        Interface::load(bram, "save.ram");
      }
      if(mode == "lower") {
        storage = Storage::LowerRAM;
        Interface::load(bram, "save.ram");
      }
    }

    if(auto fp = pak->read("save.eeprom")) {
      auto mode = fp->attribute("mode");
      if(mode == "M28C16") {
        storage = Storage::M28C16;
        Interface::load(bram, "save.eeprom");
      }
      if(mode == "M24C01") {
        storage = Storage::M24C01;
        fp->read({m24cxx.memory, 128});
      }
      if(mode == "M24C02" || mode == "X24C02") {
        storage = Storage::M24C02;
        fp->read({m24cxx.memory, 256});
      }
      if(mode == "M24C04") {
        storage = Storage::M24C04;
        fp->read({m24cxx.memory, 512});
      }
      if(mode == "M24C08") {
        storage = Storage::M24C08;
        fp->read({m24cxx.memory, 1024});
      }
      if(mode == "M24C16") {
        storage = Storage::M24C16;
        fp->read({m24cxx.memory, 2048});
      }
      rsda = fp->attribute("rsda").natural();
      wsda = fp->attribute("wsda").natural();
      wscl = fp->attribute("wscl").natural();
    }
  }

  auto save() -> void override {
    if(auto fp = pak->write("save.ram")) {
      switch(storage) {
      case Storage::WordRAM:  Interface::save(wram, "save.ram"); break;
      case Storage::UpperRAM: Interface::save(bram, "save.ram"); break;
      case Storage::LowerRAM: Interface::save(bram, "save.ram"); break;
      }
    }

    if(auto fp = pak->write("save.eeprom")) {
      switch(storage) {
      case Storage::M28C16:
        Interface::save(bram, "save.eeprom");
        break;
      case Storage::M24C01:
      case Storage::M24C02:
      case Storage::M24C04:
      case Storage::M24C08:
      case Storage::M24C16:
        fp->write({m24cxx.memory, m24cxx.size()});
        break;
      }
    }
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    if(address >= 0x200000) {
      switch(storage) {
      case Storage::WordRAM:  return data = wram[address >> 1];
      case Storage::UpperRAM: return data = bram[address >> 1] * 0x0101;
      case Storage::LowerRAM: return data = bram[address >> 1] * 0x0101;
      case Storage::M28C16:   return data = bram[address >> 1] * 0x0101;
      case Storage::M24C01:
      case Storage::M24C02:
      case Storage::M24C04:
      case Storage::M24C08:
      case Storage::M24C16:
        if(upper && rsda >> 3 == 1) data.bit(rsda) = m24cxx.read();
        if(lower && rsda >> 3 == 0) data.bit(rsda) = m24cxx.read();
        return data;
      }
    }
    return m32x.readExternal(upper, lower, address, data);
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
    if(address >= 0x200000) {
      switch(storage) {
      case Storage::WordRAM:
        if(upper) wram[address >> 1].byte(1) = data.byte(1);
        if(lower) wram[address >> 1].byte(0) = data.byte(0);
        return;
      case Storage::UpperRAM:
        if(upper) bram[address >> 1] = data;
        return;
      case Storage::LowerRAM:
        if(lower) bram[address >> 1] = data;
        return;
      case Storage::M28C16:
        if(lower) bram[address >> 1] = data;
        return;
      case Storage::M24C01:
      case Storage::M24C02:
      case Storage::M24C04:
      case Storage::M24C08:
      case Storage::M24C16: {
        maybe<n1> scl;
        maybe<n1> sda;
        if(upper && wscl >> 3 == 1) scl = data.bit(wscl);
        if(upper && wsda >> 3 == 1) sda = data.bit(wsda);
        if(lower && wscl >> 3 == 0) scl = data.bit(wscl);
        if(lower && wsda >> 3 == 0) sda = data.bit(wsda);
        return m24cxx.write(scl, sda);
      }
      }
    }
    return m32x.writeExternal(upper, lower, address, data);
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    return data = m32x.readExternalIO(upper, lower, address, data);
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
    switch(storage) {
    case Storage::M24C01: m24cxx.power(M24Cxx::Type::M24C01); break;
    case Storage::M24C02: m24cxx.power(M24Cxx::Type::M24C02); break;
    case Storage::M24C04: m24cxx.power(M24Cxx::Type::M24C04); break;
    case Storage::M24C08: m24cxx.power(M24Cxx::Type::M24C08); break;
    case Storage::M24C16: m24cxx.power(M24Cxx::Type::M24C16); break;
    }
  }

  auto serialize(serializer& s) -> void override {
    s((u32&)storage);
    s(wram);
    s(bram);
    switch(storage) {
    case Storage::M24C01: s(m24cxx); break;
    case Storage::M24C02: s(m24cxx); break;
    case Storage::M24C04: s(m24cxx); break;
    case Storage::M24C08: s(m24cxx); break;
    case Storage::M24C16: s(m24cxx); break;
    }
    s(rsda);
    s(wsda);
    s(wscl);
  }

  Storage storage = Storage::None;
  n4 rsda = 0;
  n4 wsda = 0;
  n4 wscl = 0;
};
