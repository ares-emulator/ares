struct MBC5 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  Node::Input::Rumble rumble;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
    rumble = cartridge.node->append<Node::Input::Rumble>("Rumble");
  }

  auto save() -> void override {
    Interface::save(ram, "save.ram");
  }

  auto unload() -> void override {
    cartridge.node->remove(rumble);
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x3fff) {
      return rom.read((n14)address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return rom.read(io.rom.bank << 14 | (n14)address);
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram || !io.ram.enable) return 0xff;
      return ram.read(io.ram.bank << 13 | (n13)address);
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x1fff) {
      io.ram.enable = data.bit(0,3) == 0x0a;
      return;
    }

    if(address >= 0x2000 && address <= 0x2fff) {
      io.rom.bank.bit(0,7) = data.bit(0,7);
      return;
    }

    if(address >= 0x3000 && address <= 0x3fff) {
      io.rom.bank.bit(8) = data.bit(0);
      return;
    }

    if(address >= 0x4000 && address <= 0x5fff) {
      //todo: add rumble timeout?
      rumble->setEnable(data.bit(3));
      platform->input(rumble);
      io.ram.bank = data.bit(0,3);
      return;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram || !io.ram.enable) return;
      return ram.write(io.ram.bank << 13 | (n13)address, data);
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(io.rom.bank);
    s(io.ram.enable);
    s(io.ram.bank);
  }

  struct IO {
    struct ROM {
      n9 bank = 0x01;
    } rom;
    struct RAM {
      n1 enable;
      n4 bank;
    } ram;
  } io;
};
