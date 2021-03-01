struct Linear : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
  }

  auto save() -> void override {
    Interface::save(ram, "save.ram");
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x7fff) {
      return rom.read((n15)address);
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return data;
      return ram.read((n13)address);
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
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
