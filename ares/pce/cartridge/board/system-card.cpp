struct SystemCard : Interface {
  using Interface::Interface;
  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;
  double revision;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    revision = 1.0;
    if(board.string() == "System Card 1.00") revision = 1.00;
    if(board.string() == "System Card 2.00") revision = 2.00;
    if(board.string() == "System Card 2.10") revision = 2.10;
    if(board.string() == "System Card 3.00") revision = 3.00;

    Interface::load(rom, board["memory(type=ROM,content=Program)"]);
    if(revision >= 3.00) {
      Interface::load(ram, board["memory(type=RAM,content=Work)"]);
    }
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    if(revision >= 3.00) {
      Interface::save(ram, board["memory(type=RAM,content=Work)"]);
    }
  }

  auto read(uint8 bank, uint13 address) -> uint8 override {
    uint8 data = 0xff;

    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read((uint6)bank << 13 | address);
    }

    if(revision >= 3.00 && bank >= 0x68 && bank <= 0x7f) {
      return ram.read((bank - 0x68) << 13 | address);
    }

    if(revision >= 3.00 && bank == 0xff) {
      switch(address) {
      case 0x18c4: return 0x00;
      case 0x18c5: return Region::NTSCJ() ? 0xaa : 0x55;
      case 0x18c6: return Region::NTSCJ() ? 0x55 : 0xaa;
      case 0x18c7: return ram.size() / 64_KiB;
      }
    }

    return data;
  }

  auto write(uint8 bank, uint13 address, uint8 data) -> void override {
    if(revision >= 3.00 && bank >= 0x68 && bank <= 0x7f) {
      return ram.write((bank - 0x68) << 13 | address, data);
    }
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    if(revision >= 3.00) {
      ram.serialize(s);
    }
  }
};
