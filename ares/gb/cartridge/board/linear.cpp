struct Linear : Interface {
  using Interface::Interface;
  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);
    Interface::load(ram, board["memory(type=RAM,content=Save)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(ram, board["memory(type=RAM,content=Save)"]);
  }

  auto unload() -> void override {
  }

  auto read(uint16 address, uint8 data) -> uint8 override {
    if(address >= 0x0000 && address <= 0x7fff) {
      return rom.read((uint15)address);
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return data;
      return ram.read((uint13)address);
    }

    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return;
      return ram.write(address, data);
    }
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
  }
};
