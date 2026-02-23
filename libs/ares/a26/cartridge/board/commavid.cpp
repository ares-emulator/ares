struct Commavid : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address) -> n8 override {
    if(address.bit(12)) {
      if(address >= 0x1000 && address <= 0x13ff) return ram.read(address & 0x3ff);
      return rom.read(address & 0xfff);
    }

    return 0xff;
  }
   
  auto write(n16 address, n8 data) -> bool override {
    if(address >= 0x1400 && address <= 0x17ff) {
      ram.write(address & 0x3ff, data);
      return true;
    }

    return false;
  }

  auto power() -> void override {
    ram.allocate(1_KiB);
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
  }
};
