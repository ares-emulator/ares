struct MBC2 : Interface {
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
      return rom.read(io.rom.bank << 14 | (n14)address);
    }

    if(address >= 0xa000 && address <= 0xa1ff) {
      if(!ram || !io.ram.enable) return 0xff;
      auto pair = ram.read(address.bit(1,8));
      if((address & 1) == 0) data = 0xf0 | pair.bit(0,3);
      if((address & 1) == 1) data = 0xf0 | pair.bit(4,7);
      return data;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x1fff) {
      if(!address.bit(8)) io.ram.enable = data.bit(0,3) == 0x0a;
      return;
    }

    if(address >= 0x2000 && address <= 0x3fff) {
      if(address.bit(8)) io.rom.bank = data.bit(0,3);
      if(!io.rom.bank) io.rom.bank = 0x01;
      return;
    }

    if(address >= 0xa000 && address <= 0xa1ff) {
      if(!ram || !io.ram.enable) return;
      auto pair = ram.read(address.bit(1,8));
      if((address & 1) == 0) pair.bit(0,3) = data.bit(0,3);
      if((address & 1) == 1) pair.bit(4,7) = data.bit(0,3);
      return ram.write(address.bit(1,8), pair);
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(io.rom.bank);
    s(io.ram.enable);
  }

  struct IO {
    struct ROM {
      n8 bank = 0x01;
    } rom;
    struct RAM {
      n1 enable = 0;
    } ram;
  } io;
};
