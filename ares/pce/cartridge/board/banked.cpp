struct Banked : Interface {
  using Interface::Interface;
  Memory::Readable<uint8> rom;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);
  }

  auto save(Markup::Node document) -> void override {
  }

  auto unload() -> void override {
  }

  auto read(uint8 bank, uint13 address, uint8 data) -> uint8 override {
    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(bank << 13 | address);
    }

    if(bank >= 0x40 && bank <= 0x7f) {
      return rom.read(1 + romBank << 19 | (uint6)bank << 13 | address);
    }

    return data;
  }

  auto write(uint8 bank, uint13 address, uint8 data) -> void override {
    if(bank == 0x00 && address >= 0x01ff0 && address <= 0x01ffe) {
      romBank = address.bit(0,1);
    }
  }

  auto power() -> void override {
    romBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s.integer(romBank);
  }

  uint2 romBank;
};
