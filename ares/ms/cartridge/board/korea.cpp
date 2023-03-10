struct Korea : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address <= 0x7fff) return rom.read(address);
    if(address <= 0xbfff) return rom.read(romBank << 14 | (n14)address);
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0xa000 && address <= 0xbfff) {
      romBank = data;
      return;
    }
  }

  auto power() -> void override {
    romBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
  }

  n8 romBank;
};
