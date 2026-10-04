struct ThreeEPlus : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n1 ramSelected[4];
  n6 bank[4];

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;

    auto segment = address.bit(10, 11);
    auto offset = address & 0x03ff;
    if(!ramSelected[segment]) return rom.read((u32)(bank[segment] % (rom.size() >> 10)) * 0x400 + offset);

    auto ramOffset = (u32)bank[segment] * 0x200 + (offset & 0x01ff);
    if(!(offset & 0x0200)) return ram.read(ramOffset);
    ram.write(ramOffset, data);
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address == 0x003e || address == 0x003f) {
      auto segment = data.bit(6, 7);
      ramSelected[segment] = address == 0x003e;
      bank[segment] = data.bit(0, 5);
      return data;
    }

    if(!address.bit(12)) return data;
    auto segment = address.bit(10, 11);
    auto offset = address & 0x03ff;
    if(ramSelected[segment] && (offset & 0x0200)) {
      ram.write((u32)bank[segment] * 0x200 + (offset & 0x01ff), data);
      return data;
    }
    return data;
  }

  auto power(bool reset) -> void override {
    for(auto segment : range(4)) {
      ramSelected[segment] = 0;
      bank[segment] = 0;
    }
    ram.allocate(32_KiB, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    for(auto segment : range(4)) {
      s(ramSelected[segment]);
      s(bank[segment]);
    }
    s(ram);
  }
};
