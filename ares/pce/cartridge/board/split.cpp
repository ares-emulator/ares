struct Split : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

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

  auto read(n8 bank, n13 address, n8 data) -> n8 override {
    n19 linear = (n6)bank << 13 | address;

    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(romAddress[0] + linear);
    }

    if(bank >= 0x40 && bank <= 0x7f) {
      return rom.read(romAddress[1] + linear);
    }

    return data;
  }

  auto write(n8 bank, n13 address, n8 data) -> void override {
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
  }

  n20 romAddress[2];
};
