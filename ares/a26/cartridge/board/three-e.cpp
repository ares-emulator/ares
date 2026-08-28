struct ThreeE : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n1 ramSelected;
  n8 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    if(address.bit(11)) return rom.read(rom.size() - 0x800 + (address & 0x07ff));
    if(!ramSelected) return rom.read(bank * 0x800 + (address & 0x07ff));

    auto offset = bank * 0x400 + (address & 0x03ff);
    if(!address.bit(10)) return ram.read(offset);
    ram.write(offset, data);
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address == 0x003e) {
      ramSelected = 1;
      bank = data & 31;
      return data;
    }
    if(address == 0x003f) {
      ramSelected = 0;
      bank = data % (rom.size() >> 11);
      return data;
    }
    if(ramSelected && address >= 0x1400 && address <= 0x17ff) {
      ram.write(bank * 0x400 + (address & 0x03ff), data);
      return data;
    }
    return data;
  }

  auto power(bool reset) -> void override {
    //Stella powers bank 0 and MAME the final bank.
    //Boulder Dash Demo 2 selects bank 1 before using the lower window.
    ramSelected = 0;
    bank = 1;
    ram.allocate(32_KiB, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    s(ramSelected);
    s(bank);
    s(ram);
  }
};
