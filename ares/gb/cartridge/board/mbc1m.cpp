//MBC1 with mapper bits repurposed for supporting multi-game cartridges
struct MBC1M : Interface {
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
      if(io.mode == 0) return rom.read((n14)address);
      return rom.read(io.rom.bank.bit(4,5) << 18 | (n14)address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return rom.read(io.rom.bank << 14 | (n14)address);
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return data;
      return ram.read((n13)address);
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x2000 && address <= 0x3fff) {
      io.rom.bank.bit(0,3) = data.bit(0,3);
    }

    if(address >= 0x4000 && address <= 0x5fff) {
      io.rom.bank.bit(4,5) = data.bit(0,1);
    }

    if(address >= 0x6000 && address <= 0x7fff) {
      io.mode = data.bit(0);
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return;
      ram.write((n14)address, data);
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(io.mode);
    s(io.rom.bank);
  }

  struct IO {
    n1 mode;
    struct ROM {
      n6 bank = 0x01;
    } rom;
  } io;
};
