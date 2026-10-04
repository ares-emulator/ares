struct CbsRamPlus : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n2 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x1ff8 && address <= 0x1ffa) bank = address - 0x1ff8;
    if(address >= 0x1000 && address <= 0x10ff) {
      ram.write(address & 0xff, data);
      return data;
    }
    if(address >= 0x1100 && address <= 0x11ff) return ram.read(address & 0xff);
    if(address.bit(12)) return rom.read((bank * 0x1000) + (address & 0x0fff));
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    if(address >= 0x1ff8 && address <= 0x1ffa) bank = address - 0x1ff8;
    if(address >= 0x1000 && address <= 0x10ff) {
      ram.write(address & 0xff, data);
      return data;
    }
    return data;
  }

  auto power(bool reset) -> void override {
    bank = 2;
    ram.allocate(256, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(ram);
  }
};
