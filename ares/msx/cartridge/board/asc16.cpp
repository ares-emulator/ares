//ASCII 16kbit

struct ASC16 : Interface {
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
    if(address >= 0x4000 && address <= 0x7fff) data = rom.read(bank[0] << 14 | (uint14)address);
    if(address >= 0x8000 && address <= 0xbfff) data = rom.read(bank[1] << 14 | (uint14)address);

    if(address >= 0xc000 && address <= 0xffff) data = rom.read(bank[0] << 14 | (uint14)address);
    if(address >= 0x0000 && address <= 0x3fff) data = rom.read(bank[1] << 14 | (uint14)address);

    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    if(address >= 0x6000 && address <= 0x6fff) bank[0] = data;
    if(address >= 0x7000 && address <= 0xbfff) bank[1] = data;
  }

  auto power() -> void override {
    bank[0] = 0x0f;  //R-Type = 0x0f; others = 0x00
    bank[1] = 0x00;
  }

  auto serialize(serializer& s) -> void override {
    s.array(bank);
  }

  uint8 bank[2];
};
