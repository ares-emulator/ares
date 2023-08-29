
// TODO: Game 6 and 13 do not work, posibly others also fail.

struct K188in1: Interface {
  using Interface::Interface;
  Memory::Readable < n8 > rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if (address <= 0x3fff) // static
      return rom.read((address & 0x3fff));
    if (address <= 0xbfff) // banked
      return rom.read((address-0x4000)+(romBank*0x2000));

    return data;
  }
  
  auto write(n16 address, n8 data) -> void override {
    if ((address & 0x6000) == 0x2000) {
      romBank = data ^ 0x1F;
    }
  }

  auto power() -> void override {
    romBank = 0;
  }

  auto serialize(serializer & s) -> void override {
    s(romBank);
  }

  n16 romBank;
};
