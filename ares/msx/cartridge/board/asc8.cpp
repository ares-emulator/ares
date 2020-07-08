//ASCII 8kbit

struct ASC8 : Interface {
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
    if(address >= 0x4000 && address <= 0x5fff) data = rom.read(bank[0] << 13 | (uint13)address);
    if(address >= 0x6000 && address <= 0x7fff) data = rom.read(bank[1] << 13 | (uint13)address);
    if(address >= 0x8000 && address <= 0x9fff) data = rom.read(bank[2] << 13 | (uint13)address);
    if(address >= 0xa000 && address <= 0xbfff) data = rom.read(bank[3] << 13 | (uint13)address);

    if(address >= 0xc000 && address <= 0xdfff) data = rom.read(bank[0] << 13 | (uint13)address);
    if(address >= 0xe000 && address <= 0xffff) data = rom.read(bank[1] << 13 | (uint13)address);
    if(address >= 0x0000 && address <= 0x1fff) data = rom.read(bank[2] << 13 | (uint13)address);
    if(address >= 0x2000 && address <= 0x3fff) data = rom.read(bank[3] << 13 | (uint13)address);

    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    if(address >= 0x6000 && address <= 0x67ff) bank[0] = data;
    if(address >= 0x6800 && address <= 0x6fff) bank[1] = data;
    if(address >= 0x7000 && address <= 0x77ff) bank[2] = data;
    if(address >= 0x7800 && address <= 0x7fff) bank[3] = data;
  }

  auto power() -> void override {
    bank[0] = 0;
    bank[1] = 0;
    bank[2] = 0;
    bank[3] = 0;
  }

  auto serialize(serializer& s) -> void override {
    s.array(bank);
  }

  uint8 bank[4];
};
