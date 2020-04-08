struct SystemCard : Interface {
  using Interface::Interface;
  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);
    Interface::load(ram, board["memory(type=RAM,content=Work)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(ram, board["memory(type=RAM,content=Work)"]);
  }

  auto read(uint8 bank, uint13 address) -> uint8 override {
    uint8 data = 0xff;

    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read((uint6)bank << 13 | address);
    }

    if(bank >= 0x68 && bank <= 0x7f) {
      if(!ram) return data;
      return ram.read((bank - 0x68) << 13 | address);
    }

    return data;
  }

  auto write(uint8 bank, uint13 address, uint8 data) -> void override {
    if(bank >= 0x68 && bank <= 0x7f) {
      if(!ram) return;
      return ram.write((bank - 0x68) << 13 | address, data);
    }
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    if(ram) ram.serialize(s);
  }
};
