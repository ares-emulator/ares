struct Codemasters : Interface {
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
    case 0x0000 ... 0x3fff:
      return rom.read(romBank[0] << 14 | (n14)address);
    case 0x4000 ... 0x7fff:
      return rom.read(romBank[1] << 14 | (n14)address);
    case 0x8000 ... 0x9fff:
      return rom.read(romBank[2] << 14 | (n14)address);
    case 0xa000 ... 0xbfff:
      if(ram && ramEnable) return ram.read((n13)address);
      return rom.read(romBank[2] << 14 | (n14)address);
    }
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    switch(address) {
    case 0x0000 ... 0x3fff:
      romBank[0] = data;
      return;
    case 0x4000 ... 0x7fff:
      romBank[1] = data;
      ramEnable  = data.bit(7);
      return;
    case 0x8000 ... 0x9fff:
      romBank[2] = data;
      return;
    case 0xa000 ... 0xbfff:
      if(ram && ramEnable) return ram.write((n13)address, data);
      romBank[2] = data;
      return;
    }
  }

  auto power() -> void override {
    romBank[0] = 0;
    romBank[1] = 1;
    romBank[2] = 0;  //verified
    ramEnable  = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
    s(ramEnable);
  }

  n8 romBank[3];
  n1 ramEnable;
};
