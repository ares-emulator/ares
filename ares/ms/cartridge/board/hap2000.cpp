struct Hap2000: Interface {
  using Interface::Interface;
  Memory::Readable < n8 > rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {}

  auto unload() -> void override {}

  auto read(n16 address, n8 data) -> n8 override {
    if (address <= 0x3fff) // static
      return rom.read((address & 0x3fff));

    if (address < 0xbfff) // banked
      return rom.read((romBank * 0x8000) + (address));

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if (address == 0x2000) {
      romBank = data;
    }
  }

  auto power() -> void override {
    romBank = 0;
  }

  auto serialize(serializer & s) -> void override {
    s(romBank);
  }

  n8 romBank;
};
