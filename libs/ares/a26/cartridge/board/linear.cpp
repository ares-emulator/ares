struct Linear : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  bool hasRam;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address) -> n8 override {
    if(address.bit(12)) {
      if(hasRam && address >= 0x1080 && address <= 0x10ff) return ram.read(address & 0x7f);
      return rom.read(address & 0xfff);
    }

    return 0xff;
  }
   
  auto write(n16 address, n8 data) -> bool override {
    if(address >= 0x1000 && address <= 0x107f) {
      hasRam = true;
      ram.write(address & 0x7f, data);
      return true;
    }

    return false;
  }

  auto power() -> void override {
    hasRam = 0;
    ram.allocate(128);
  }

  auto serialize(serializer& s) -> void override {
    s(hasRam);
    s(ram);
  }
};
