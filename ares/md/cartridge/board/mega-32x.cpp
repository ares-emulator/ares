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
    M24C02,
    M24C04,
  };

  auto load() -> void override {
    storage = Storage::None;
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
      if(mode == "24C02" || mode == "X24C02") {
        storage = Storage::M24C02;
        fp->read({m24cxx.bytes, 256});
      }
      if(mode == "24C04") {
        storage = Storage::M24C04;
        fp->read({m24cxx.bytes, 512});
      }
      rsda = fp->attribute("rsda").natural();
      wsda = fp->attribute("wsda").natural();
      wscl = fp->attribute("wscl").natural();
    }
  }

  auto save() -> void override {
    if(auto fp = pak->write("save.ram")) {
      if(storage == Storage::WordRAM) {
        Interface::save(wram, "save.ram");
      }
      if(storage == Storage::UpperRAM) {
        Interface::save(bram, "save.ram");
      }
      if(storage == Storage::LowerRAM) {
        Interface::save(bram, "save.ram");
      }
    }

    if(auto fp = pak->write("save.eeprom")) {
      if(storage == Storage::M24C02) {
        fp->write({m24cxx.bytes, 256});
      }
      if(storage == Storage::M24C04) {
        fp->write({m24cxx.bytes, 512});
      }
    }
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    if(address >= 0x200000) {
      switch(storage) {
      case Storage::WordRAM:  return data = wram[address >> 1];
      case Storage::UpperRAM: return data = bram[address >> 1] * 0x0101;
      case Storage::LowerRAM: return data = bram[address >> 1] * 0x0101;
      case Storage::M24C02:
      case Storage::M24C04:
        data.bit(rsda) = m24cxx.read();
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
      case Storage::M24C02:
      case Storage::M24C04:
        return m24cxx.write(data.bit(wscl), data.bit(wsda));
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
    if(storage == Storage::M24C02) m24cxx.power(M24Cxx::Type::M24C02);
    if(storage == Storage::M24C04) m24cxx.power(M24Cxx::Type::M24C04);
  }

  auto serialize(serializer& s) -> void override {
    s((u32&)storage);
    s(wram);
    s(bram);
    if(storage == Storage::M24C02) s(m24cxx);
    if(storage == Storage::M24C04) s(m24cxx);
    s(rsda);
    s(wsda);
    s(wscl);
  }

  Storage storage = Storage::None;
  u8 rsda = 0;
  u8 wsda = 0;
  u8 wscl = 0;
};
