struct CPUWiz4KSC : Interface {
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

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address < 0x1000) return data;
    if(address <= 0x107f) {
      ram.write(address & 0x7f, data);
      return data;
    }
    if(address <= 0x10ff) return ram.read(address & 0x7f);
    return rom.read(address & 0xfff);
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address < 0x1000 || address > 0x107f) return data;
    ram.write(address & 0x7f, data);
    return data;
  }

  auto power(bool reset) -> void override {
    ram.allocate(128, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
  }
};
