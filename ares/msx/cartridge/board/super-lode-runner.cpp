//Super Lode Runner
//(not working: requires MSX BASIC)

struct SuperLodeRunner : Interface {
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
    if(address >= 0x8000 && address <= 0xbfff) data = rom.read(bank << 14 | (uint14)address);
    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    if(address >= 0x0000 && address <= 0x3fff) bank = data;
  }

  auto power() -> void override {
    bank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s.integer(bank);
  }

  uint8 bank;
};
