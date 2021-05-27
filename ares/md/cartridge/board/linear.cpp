struct Linear : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;
  Memory::Writable<n16> wram;
  Memory::Writable<n8 > bram;
  M24Cx m24cx;
  M24Cxx m24cxx;
  M24Cxxx m24cxxx;
  enum class Storage : u32 {
    None,
    WordRAM,
    UpperRAM,
    LowerRAM,
    M28C16,
    X24C01,
    M24C01,
    M24C02,
    M24C04,
    M24C08,
    M24C16,
    M24C32,
    M24C64,
    M24C65,
    M24C128,
    M24C256,
    M24C512,
  };

  auto load() -> void override {
    storage = Storage::None;
    m24cx.erase();
    m24cxx.erase();
    m24cxxx.erase();

    Interface::load(rom, "program.rom");

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
      if(mode == "X24C01") {
        storage = Storage::X24C01;
        fp->read({m24cx.memory, 128});
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
      if(mode == "M24C32") {
        storage = Storage::M24C32;
        fp->read({m24cxxx.memory, 4096});
      }
      if(mode == "M24C64" || mode == "M24C65") {
        storage = Storage::M24C65;
        fp->read({m24cxxx.memory, 8192});
      }
      if(mode == "M24C128") {
        storage = Storage::M24C128;
        fp->read({m24cxxx.memory, 16384});
      }
      if(mode == "M24C256") {
        storage = Storage::M24C256;
        fp->read({m24cxxx.memory, 32768});
      }
      if(mode == "M24C512") {
        storage = Storage::M24C512;
        fp->read({m24cxxx.memory, 65536});
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
      case Storage::X24C01:
        fp->write({m24cx.memory, m24cx.size()});
        break;
      case Storage::M24C01:
      case Storage::M24C02:
      case Storage::M24C04:
      case Storage::M24C08:
      case Storage::M24C16:
        fp->write({m24cxx.memory, m24cxx.size()});
        break;
      case Storage::M24C32:
      case Storage::M24C64:
      case Storage::M24C65:
      case Storage::M24C128:
      case Storage::M24C256:
      case Storage::M24C512:
        fp->write({m24cxxx.memory, m24cxxx.size()});
        break;
      }
    }
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    if(address >= 0x200000 && ramEnable) {
      switch(storage) {
      case Storage::WordRAM:  return data = wram[address >> 1];
      case Storage::UpperRAM: return data = bram[address >> 1] * 0x0101;
      case Storage::LowerRAM: return data = bram[address >> 1] * 0x0101;
      case Storage::M28C16:   return data = bram[address >> 1] * 0x0101;
      case Storage::X24C01:
        if(upper && rsda >> 3 == 1) data.bit(rsda) = m24cx.read();
        if(lower && rsda >> 3 == 0) data.bit(rsda) = m24cx.read();
        return data;
      case Storage::M24C01:
      case Storage::M24C02:
      case Storage::M24C04:
      case Storage::M24C08:
      case Storage::M24C16:
        if(upper && rsda >> 3 == 1) data.bit(rsda) = m24cxx.read();
        if(lower && rsda >> 3 == 0) data.bit(rsda) = m24cxx.read();
        return data;
      case Storage::M24C32:
      case Storage::M24C64:
      case Storage::M24C65:
      case Storage::M24C128:
      case Storage::M24C256:
      case Storage::M24C512:
        if(upper && rsda >> 3 == 1) data.bit(rsda) = m24cxxx.read();
        if(lower && rsda >> 3 == 0) data.bit(rsda) = m24cxxx.read();
        return data;
      }
    }
    return data = rom[address >> 1];
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
    if(address >= 0x200000 && ramEnable) {
      //emulating ramWritable will break commercial software:
      //it does not appear that many (any?) games actually connect $a130f1.d1 to /WE;
      //hence RAM ends up always being writable, and many games fail to set d1=1
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
      case Storage::X24C01: {
        maybe<n1> scl;
        maybe<n1> sda;
        if(upper && wscl >> 3 == 1) scl = data.bit(wscl);
        if(upper && wsda >> 3 == 1) sda = data.bit(wsda);
        if(lower && wscl >> 3 == 0) scl = data.bit(wscl);
        if(lower && wsda >> 3 == 0) sda = data.bit(wsda);
        return m24cx.write(scl, sda);
      }
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
      case Storage::M24C32:
      case Storage::M24C64:
      case Storage::M24C65:
      case Storage::M24C128:
      case Storage::M24C256:
      case Storage::M24C512: {
        maybe<n1> scl;
        maybe<n1> sda;
        if(upper && wscl >> 3 == 1) scl = data.bit(wscl);
        if(upper && wsda >> 3 == 1) sda = data.bit(wsda);
        if(lower && wscl >> 3 == 0) scl = data.bit(wscl);
        if(lower && wsda >> 3 == 0) sda = data.bit(wsda);
        return m24cxxx.write(scl, sda);
      }
      }
    }
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    return data;
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(!lower) return;  //todo: unconfirmed
    if(address == 0xa130f0) {
      ramEnable   = data.bit(0);
      ramWritable = data.bit(1);
    }
  }

  auto power(bool reset) -> void override {
    ramEnable = 1;
    ramWritable = 1;
    switch(storage) {
    case Storage::X24C01: m24cx.power(); break;
    case Storage::M24C01: m24cxx.power(M24Cxx::Type::M24C01); break;
    case Storage::M24C02: m24cxx.power(M24Cxx::Type::M24C02); break;
    case Storage::M24C04: m24cxx.power(M24Cxx::Type::M24C04); break;
    case Storage::M24C08: m24cxx.power(M24Cxx::Type::M24C08); break;
    case Storage::M24C16: m24cxx.power(M24Cxx::Type::M24C16); break;
    case Storage::M24C32: m24cxxx.power(M24Cxxx::Type::M24C32); break;
    case Storage::M24C64: m24cxxx.power(M24Cxxx::Type::M24C64); break;
    case Storage::M24C65: m24cxxx.power(M24Cxxx::Type::M24C65); break;
    case Storage::M24C128: m24cxxx.power(M24Cxxx::Type::M24C128); break;
    case Storage::M24C256: m24cxxx.power(M24Cxxx::Type::M24C256); break;
    case Storage::M24C512: m24cxxx.power(M24Cxxx::Type::M24C512); break;
    }
  }

  auto serialize(serializer& s) -> void override {
    s((u32&)storage);
    s(wram);
    s(bram);
    switch(storage) {
    case Storage::X24C01: s(m24cx); break;
    case Storage::M24C01: s(m24cxx); break;
    case Storage::M24C02: s(m24cxx); break;
    case Storage::M24C04: s(m24cxx); break;
    case Storage::M24C08: s(m24cxx); break;
    case Storage::M24C16: s(m24cxx); break;
    case Storage::M24C32: s(m24cxxx); break;
    case Storage::M24C64: s(m24cxxx); break;
    case Storage::M24C65: s(m24cxxx); break;
    case Storage::M24C128: s(m24cxxx); break;
    case Storage::M24C256: s(m24cxxx); break;
    case Storage::M24C512: s(m24cxxx); break;
    }
    s(ramEnable);
    s(ramWritable);
    s(rsda);
    s(wsda);
    s(wscl);
  }

  Storage storage = Storage::None;
  n1 ramEnable = 1;
  n1 ramWritable = 1;
  n4 rsda = 0;
  n4 wsda = 0;
  n4 wscl = 0;
};
