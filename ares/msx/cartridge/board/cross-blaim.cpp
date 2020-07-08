//Cross Blaim

struct CrossBlaim : Interface {
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
    if(mode == 0 || mode == 1) {
      if(address >> 14 == 0) data = rom.read((uint14)address + 16_KiB);
      if(address >> 14 == 1) data = rom.read((uint14)address);
      if(address >> 14 == 2) data = rom.read((uint14)address + 16_KiB);
      if(address >> 14 == 3) data = rom.read((uint14)address + 16_KiB);
    }

    if(mode == 2) {
      if(address >> 14 == 0) data = 0xff;
      if(address >> 14 == 1) data = rom.read((uint14)address);
      if(address >> 14 == 2) data = rom.read((uint14)address + 32_KiB);
      if(address >> 14 == 3) data = 0xff;
    }

    if(mode == 3) {
      if(address >> 14 == 0) data = 0xff;
      if(address >> 14 == 1) data = rom.read((uint14)address);
      if(address >> 14 == 2) data = rom.read((uint14)address + 48_KiB);
      if(address >> 14 == 3) data = 0xff;
    }

    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    mode = data;
  }

  auto power() -> void override {
    mode = 0;
  }

  auto serialize(serializer& s) -> void override {
    s.integer(mode);
  }

  uint2 mode;
};
