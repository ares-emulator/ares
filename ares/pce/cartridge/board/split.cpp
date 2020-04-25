struct Split : Interface {
  using Interface::Interface;
  Memory::Readable<uint8> rom;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);

    //every licensed HuCard follows this format;
    //but ideally the manifest should list two separate ROMs ...
    romAddress[0] = 0;
    romAddress[1] = bit::round(rom.size()) >> 1;
  }

  auto save(Markup::Node document) -> void override {
  }

  auto unload() -> void override {
  }

  auto read(uint8 bank, uint13 address, uint8 data) -> uint8 override {
    uint19 linear = (uint6)bank << 13 | address;

    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(romAddress[0] + linear);
    }

    if(bank >= 0x40 && bank <= 0x7f) {
      return rom.read(romAddress[1] + linear);
    }

    return data;
  }

  auto write(uint8 bank, uint13 address, uint8 data) -> void override {
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
  }

  uint20 romAddress[2];
};
