struct MBC1 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
  }

  auto save() -> void override {
    Interface::save(ram, "save.ram");
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x3fff) {
      return rom.read((n14)address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      if(io.mode == 0) {
        return rom.read(io.ram.bank << 19 | io.rom.bank << 14 | (n14)address);
      } else {
        return rom.read(io.rom.bank << 14 | (n14)address);
      }
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram || !io.ram.enable) return 0xff;
      if(io.mode == 0) {
        return ram.read((n13)address);
      } else {
        return ram.read(io.ram.bank << 13 | (n13)address);
      }
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x1fff) {
      io.ram.enable = data.bit(0,3) == 0x0a;
      return;
    }

    if(address >= 0x2000 && address <= 0x3fff) {
      io.rom.bank = data.bit(0,4);
      if(!io.rom.bank) io.rom.bank = 0x01;
      return;
    }

    if(address >= 0x4000 && address <= 0x5fff) {
      io.ram.bank = data.bit(0,1);
      return;
    }

    if(address >= 0x6000 && address <= 0x7fff) {
      io.mode = data.bit(0);
      return;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram || !io.ram.enable) return;
      if(io.mode == 0) {
        return ram.write((n13)address, data);
      } else {
        return ram.write(io.ram.bank << 13 | (n13)address, data);
      }
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(io.mode);
    s(io.rom.bank);
    s(io.ram.enable);
    s(io.ram.bank);
  }

  struct IO {
    n1 mode;
    struct ROM {
      n8 bank = 0x01;
    } rom;
    struct RAM {
      n1 enable;
      n8 bank;
    } ram;
  } io;
};
