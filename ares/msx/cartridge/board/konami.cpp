//Konami (without Sound Creative Chip)

struct Konami : Interface {
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
    if(address >> 13 == 2) data = rom.read(bank[0] << 13 | (uint13)address);
    if(address >> 13 == 3) data = rom.read(bank[1] << 13 | (uint13)address);
    if(address >> 13 == 4) data = rom.read(bank[2] << 13 | (uint13)address);
    if(address >> 13 == 5) data = rom.read(bank[3] << 13 | (uint13)address);
    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    if(address >> 13 == 2) bank[0] = 0;
    if(address >> 13 == 3) bank[1] = data;
    if(address >> 13 == 4) bank[2] = data;
    if(address >> 13 == 5) bank[3] = data;
  }

  auto power() -> void override {
    bank[0] = 0;
    bank[1] = 1;
    bank[2] = 0;
    bank[3] = 0;
  }

  auto serialize(serializer& s) -> void override {
    s.array(bank);
  }

  uint8 bank[4];
};
