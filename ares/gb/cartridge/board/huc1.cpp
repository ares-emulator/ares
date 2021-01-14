struct HuC1 : Interface {
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
    if(address >= 0x0000 && address <= 0x3fff) {
      return rom.read((uint14)address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return rom.read(io.rom.bank << 14 | (uint14)address);
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return 0xff;
      return ram.read(io.ram.bank << 13 | (uint13)address);
    }

    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    if(address >= 0x0000 && address <= 0x1fff) {
      io.ram.writable = data.bit(0,3) == 0x0a;
      return;
    }

    if(address >= 0x2000 && address <= 0x3fff) {
      io.rom.bank = data;
      if(!io.rom.bank) io.rom.bank = 0x01;
      return;
    }

    if(address >= 0x4000 && address <= 0x5fff) {
      io.ram.bank = data;
      return;
    }

    if(address >= 0x6000 && address <= 0x7fff) {
      io.model = data.bit(0);
      return;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram || !io.ram.writable) return;
      return ram.write(io.ram.bank << 13 | (uint13)address, data);
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(io.model);
    s(io.rom.bank);
    s(io.ram.writable);
    s(io.ram.bank);
  }

  struct IO {
    uint1 model;
    struct ROM {
      uint8 bank = 0x01;
    } rom;
    struct RAM {
      uint1 writable;
      uint8 bank;
    } ram;
  } io;
};
