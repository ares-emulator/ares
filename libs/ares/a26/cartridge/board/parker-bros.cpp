struct ParkerBros : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n3 banks[3];

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address) -> n8 override {
    bankswitch(address);

    if(address.bit(12)) {
      if(address >= 0x1000 && address <= 0x13ff) return rom.read((banks[0] * 0x400) + (address & 0x3ff));
      if(address >= 0x1400 && address <= 0x17ff) return rom.read((banks[1] * 0x400) + (address & 0x3ff));
      if(address >= 0x1800 && address <= 0x1bff) return rom.read((banks[2] * 0x400) + (address & 0x3ff));
      if(address >= 0x1c00 && address <= 0x1fff) return rom.read((      7  * 0x400) + (address & 0x3ff));
    }

    return 0xff;
  }
   
  auto write(n16 address, n8 data) -> bool override {
    bankswitch(address);
    return false;
  }

  auto bankswitch(n16 address) -> void {
    if(address >= 0x1fe0 && address <= 0x1fe7) banks[0] = address - 0x1fe0;
    if(address >= 0x1fe8 && address <= 0x1fef) banks[1] = address - 0x1fe8;
    if(address >= 0x1ff0 && address <= 0x1ff7) banks[2] = address - 0x1ff0;
  }

  auto power() -> void override {
    banks[0] = 0;
    banks[1] = 1;
    banks[2] = 2;
  }

  auto serialize(serializer& s) -> void override {
    s(banks[0]);
    s(banks[1]);
    s(banks[2]);
  }
};
