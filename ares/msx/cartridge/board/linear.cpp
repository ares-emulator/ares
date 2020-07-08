struct Linear : Interface {
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

  auto read(uint16 address, uint8 data) -> uint8 override {
    data = rom.read(address);
    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
  }
};
