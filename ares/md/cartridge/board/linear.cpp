struct Linear : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;
  Memory::Writable<n16> wram;
  Memory::Writable<n8 > bram;
  X24C01 x24c01;
  M24Cxx m24cxx;
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
    M24C65,
  };

  auto load() -> void override {
    storage = Storage::None;
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
      if(mode == "28C16") {
        storage = Storage::M28C16;
        Interface::load(bram, "save.eeprom");
      }
      if(mode == "X24C01") {
        storage = Storage::X24C01;
        fp->read({x24c01.bytes, 128});
      }
      if(mode == "24C01") {
        storage = Storage::M24C01;
        fp->read({m24cxx.bytes, 128});
      }
      if(mode == "24C02" || mode == "X24C02") {
        storage = Storage::M24C02;
        fp->read({m24cxx.bytes, 256});
      }
      if(mode == "24C04") {
        storage = Storage::M24C04;
        fp->read({m24cxx.bytes, 512});
      }
      if(mode == "24C08") {
        storage = Storage::M24C08;
        fp->read({m24cxx.bytes, 1024});
      }
      if(mode == "24C16") {
        storage = Storage::M24C16;
        fp->read({m24cxx.bytes, 2048});
      }
      if(mode == "24C65") {
        storage = Storage::M24C65;
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
      if(storage == Storage::M28C16) {
        Interface::save(bram, "save.eeprom");
      }
      if(storage == Storage::X24C01) {
        fp->write({x24c01.bytes, 128});
      }
      if(storage == Storage::M24C01) {
        fp->write({m24cxx.bytes, 128});
      }
      if(storage == Storage::M24C02) {
        fp->write({m24cxx.bytes, 256});
      }
      if(storage == Storage::M24C04) {
        fp->write({m24cxx.bytes, 512});
      }
      if(storage == Storage::M24C08) {
        fp->write({m24cxx.bytes, 1024});
      }
      if(storage == Storage::M24C16) {
        fp->write({m24cxx.bytes, 2048});
      }
      if(storage == Storage::M24C65) {
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
        data.bit(rsda) = x24c01.read();
        return data;
      case Storage::M24C01:
      case Storage::M24C02:
      case Storage::M24C04:
      case Storage::M24C08:
      case Storage::M24C16:
        data.bit(rsda) = m24cxx.read();
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
      case Storage::X24C01:
        return x24c01.write(data.bit(wscl), data.bit(wsda));
      case Storage::M24C01:
      case Storage::M24C02:
      case Storage::M24C04:
      case Storage::M24C08:
      case Storage::M24C16:
        return m24cxx.write(data.bit(wscl), data.bit(wsda));
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
    if(storage == Storage::X24C01) x24c01.power();
    if(storage == Storage::M24C01) m24cxx.power(M24Cxx::Type::M24C01);
    if(storage == Storage::M24C02) m24cxx.power(M24Cxx::Type::M24C02);
    if(storage == Storage::M24C04) m24cxx.power(M24Cxx::Type::M24C04);
    if(storage == Storage::M24C08) m24cxx.power(M24Cxx::Type::M24C08);
    if(storage == Storage::M24C16) m24cxx.power(M24Cxx::Type::M24C16);
  }

  auto serialize(serializer& s) -> void override {
    s((u32&)storage);
    s(wram);
    s(bram);
    if(storage == Storage::X24C01) s(x24c01);
    if(storage == Storage::M24C01) s(m24cxx);
    if(storage == Storage::M24C02) s(m24cxx);
    if(storage == Storage::M24C04) s(m24cxx);
    if(storage == Storage::M24C08) s(m24cxx);
    if(storage == Storage::M24C16) s(m24cxx);
    s(ramEnable);
    s(ramWritable);
    s(rsda);
    s(wsda);
    s(wscl);
  }

  Storage storage = Storage::None;
  n1 ramEnable = 1;
  n1 ramWritable = 1;
  n8 rsda = 0;
  n8 wsda = 0;
  n8 wscl = 0;
};
