struct Banked : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n8 bank, n13 address, n8 data) -> n8 override {
    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(bank << 13 | address);
    }

    if(bank >= 0x40 && bank <= 0x7f) {
      return rom.read(1 + romBank << 19 | (n6)bank << 13 | address);
    }

    return data;
  }

  auto write(n8 bank, n13 address, n8 data) -> void override {
    if(bank == 0x00 && address >= 0x01ff0 && address <= 0x01ffe) {
      romBank = address.bit(0,1);
    }
  }

  auto power() -> void override {
    romBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
  }

  n2 romBank;
};
