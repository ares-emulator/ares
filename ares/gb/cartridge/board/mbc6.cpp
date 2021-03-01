struct MBC6 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  Memory::Writable<n8> flash;  //Macronix MX29F008TC-14 (writes unemulated)

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
    Interface::load(flash, "download.flash");
  }

  auto save() -> void override {
    Interface::save(ram, "save.ram");
    Interface::save(flash, "download.flash");
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x3fff) {
      return rom.read((n14)address);
    }

    if(address >= 0x4000 && address <= 0x5fff) {
      if(io.region[0].select == 0) return   rom.read(io.region[0].bank << 13 | (n13)address);
      if(io.region[0].select == 1) return flash.read(io.region[0].bank << 13 | (n13)address);
    }

    if(address >= 0x6000 && address <= 0x7fff) {
      if(io.region[1].select == 0) return   rom.read(io.region[1].bank << 13 | (n13)address);
      if(io.region[1].select == 1) return flash.read(io.region[1].bank << 13 | (n13)address);
    }

    if(address >= 0xa000 && address <= 0xafff) {
      if(!ram || !io.ram.enable) return 0xff;
      return ram.read(io.ram.bank[0] << 12 | (n12)address);
    }

    if(address >= 0xb000 && address <= 0xbfff) {
      if(!ram || !io.ram.enable) return 0xff;
      return ram.read(io.ram.bank[1] << 12 | (n12)address);
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x03ff) {
      io.ram.enable = data.bit(0,3) == 0xa;
      return;
    }

    if(address >= 0x0400 && address <= 0x07ff) {
      io.ram.bank[0] = data.bit(0,2);
      return;
    }

    if(address >= 0x0800 && address <= 0x0bff) {
      io.ram.bank[1] = data.bit(0,2);
      return;
    }

    if(address >= 0x0c00 && address <= 0x0fff) {
      io.flash.enable = data.bit(0);
      return;
    }

    if(address >= 0x1000 && address <= 0x1fff) {
      io.flash.writable = data.bit(0);
      return;
    }

    if(address >= 0x2000 && address <= 0x27ff) {
      io.region[0].bank = data.bit(0,6);
      return;
    }

    if(address >= 0x2800 && address <= 0x2fff) {
      io.region[0].select = data.bit(3);
      return;
    }

    if(address >= 0x3000 && address <= 0x37ff) {
      io.region[1].bank = data.bit(0,6);
      return;
    }

    if(address >= 0x3800 && address <= 0x3fff) {
      io.region[1].select = data.bit(3);
      return;
    }

    if(address >= 0xa000 && address <= 0xafff) {
      if(!ram || !io.ram.enable) return;
      return ram.write(io.ram.bank[0] << 12 | (n12)address, data);
    }

    if(address >= 0xb000 && address <= 0xbfff) {
      if(!ram || !io.ram.enable) return;
      return ram.write(io.ram.bank[1] << 12 | (n12)address, data);
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(flash);
    s(io.region[0].select);
    s(io.region[0].bank);
    s(io.region[1].select);
    s(io.region[1].bank);
    s(io.ram.enable);
    s(io.ram.bank);
    s(io.flash.enable);
    s(io.flash.writable);
  }

  struct IO {
    struct Region {
      n1 select;  //0 = ROM, 1 = Flash
      n7 bank;
    } region[2];
    struct RAM {
      n1 enable;
      n3 bank[2];
    } ram;
    struct Flash {
      n1 enable;    //unknown purpose
      n1 writable;  //flash /WE pin
    } flash;
  } io;
};
