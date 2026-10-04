struct MNetwork : Interface {
  using Interface::Interface;
  static constexpr u8 LowerRam = 7;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n3 lowerSelector;
  n2 upperRamBank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto select(n16 address) -> void {
    address &= 0x1fff;
    if(address >= 0x1fe8 && address <= 0x1feb) upperRamBank = address - 0x1fe8;
    if(address < 0x1fe0 || address > 0x1fe7) return;

    auto selector = address - 0x1fe0;
    if(selector == 7) {
      lowerSelector = LowerRam;
      return;
    }
    if(rom.size() == 8_KiB) {
      if(selector >= 4) lowerSelector = selector - 4;
      return;
    }
    if(rom.size() == 12_KiB) {
      static constexpr u8 banks[] = {0, 1, 0, 1, 2, 3, 4};
      lowerSelector = banks[selector];
      return;
    }
    lowerSelector = selector;
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    select(address);

    if(address >= 0x1000 && address <= 0x13ff) {
      if(lowerSelector != LowerRam) return rom.read((lowerSelector * 0x800) + (address & 0x7ff));
      ram.write(address & 0x3ff, data);
      return data;
    }
    if(address >= 0x1400 && address <= 0x17ff) {
      if(lowerSelector != LowerRam) return rom.read((lowerSelector * 0x800) + (address & 0x7ff));
      return ram.read(address & 0x3ff);
    }
    if(address >= 0x1800 && address <= 0x18ff) {
      auto offset = 0x400 + upperRamBank * 0x100 + (address & 0xff);
      ram.write(offset, data);
      return data;
    }
    if(address >= 0x1900 && address <= 0x19ff) {
      return ram.read(0x400 + upperRamBank * 0x100 + (address & 0xff));
    }
    if(address >= 0x1a00) return rom.read((rom.size() - 0x800) + (address & 0x7ff));
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    select(address);

    if(lowerSelector == LowerRam && address >= 0x1000 && address <= 0x13ff) {
      ram.write(address & 0x3ff, data);
      return data;
    }
    if(address >= 0x1800 && address <= 0x18ff) {
      ram.write(0x400 + upperRamBank * 0x100 + (address & 0xff), data);
      return data;
    }
    return data;
  }

  auto power(bool reset) -> void override {
    lowerSelector = 0;
    upperRamBank = 0;
    ram.allocate(2_KiB, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    s(lowerSelector);
    s(upperRamBank);
    s(ram);
  }
};
