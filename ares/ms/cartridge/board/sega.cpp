struct Sega : Interface {
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
    switch(address) {
    case 0x0000 ... 0x03ff:
      return rom.read(address);
    case 0x0400 ... 0x3fff:
      return rom.read(romBankUpper << 17 | romBank0 << 14 | (n14)address);
    case 0x4000 ... 0x7fff:
      return rom.read(romBankUpper << 17 | romBank1 << 14 | (n14)address);
    case 0x8000 ... 0xbfff:
      if(ram && ramEnableBank2) {
        return ram.read(ramBank2 << 14 | (n14)address);
      }
      return rom.read(romBankUpper << 17 | romBank2 << 14 | (n14)address);
    case 0xc000 ... 0xffff:
      if(ram && ramEnableBank3) {
        return ram.read((n14)address);
      }
      return data;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    switch(address) {
    case 0xfffc:
      if(data.bit(0,1) == 0) romBankShift = 0;
      if(data.bit(0,1) == 1) romBankShift = 3;
      if(data.bit(0,1) == 2) romBankShift = 2;
      if(data.bit(0,1) == 3) romBankShift = 1;
      ramBank2       = data.bit(2);
      ramEnableBank2 = data.bit(3);
      ramEnableBank3 = data.bit(4);
      romWriteEnable = data.bit(7);
      break;
    case 0xfffd:
      romBankUpper = romBankShift;
      romBank0     = data;
      break;
    case 0xfffe:
      romBankUpper = romBankShift;
      romBank1     = data;
      break;
    case 0xffff:
      romBankUpper = romBankShift;
      romBank2     = data;
      break;
    }

    switch(address) {
    case 0x0000 ... 0x03ff:
      if(romWriteEnable) {
        return rom.write(address, data);
      }
      return;
    case 0x0400 ... 0x3fff:
      if(romWriteEnable) {
        return rom.write(romBankUpper << 17 | romBank0 << 14 | (n14)address, data);
      }
      return;
    case 0x4000 ... 0x7fff:
      if(romWriteEnable) {
        return rom.write(romBankUpper << 17 | romBank1 << 14 | (n14)address, data);
      }
      return;
    case 0x8000 ... 0xbfff:
      if(ram && ramEnableBank2) {
        return ram.write(ramBank2 << 14 | (n14)address, data);
      }
      if(romWriteEnable) {
        return rom.write(romBankUpper << 17 | romBank2 << 14 | (n14)address, data);
      }
      return;
    case 0xc000 ... 0xffff:
      if(ram && ramEnableBank3) {
        return ram.write((n14)address, data);
      }
      return;
    }
  }

  auto power() -> void override {
    romBankUpper   = 0;
    romBankShift   = 0;
    ramBank2       = 0;
    ramEnableBank2 = 0;
    ramEnableBank3 = 0;
    romWriteEnable = 0;
    romBank0       = 0;
    romBank1       = 1;
    romBank2       = 2;
  }

  auto serialize(serializer& s) -> void override {
    s(romBankUpper);
    s(romBankShift);
    s(ramBank2);
    s(ramEnableBank2);
    s(ramEnableBank3);
    s(romWriteEnable);
    s(romBank0);
    s(romBank1);
    s(romBank2);
  }

  //$fffc
  n2 romBankUpper;    //latched romBankShift
  n2 romBankShift;    //for 512K mappers
  n1 ramBank2;
  n1 ramEnableBank2;
  n1 ramEnableBank3;
  n1 romWriteEnable;  //for development hardware with RAM in place of ROM

  //$fffd
  n8 romBank0;

  //$fffe
  n8 romBank1;

  //$ffff
  n8 romBank2;
};
