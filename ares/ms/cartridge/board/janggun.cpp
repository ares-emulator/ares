struct Janggun : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    switch(address) {
    case 0x0000 ... 0x1fff: return rom.read(romBank[0] << 13 | (n13)address);
    case 0x2000 ... 0x3fff: return rom.read(romBank[1] << 13 | (n13)address);
    case 0x4000 ... 0x5fff: return rom.read(romBank[2] << 13 | (n13)address);
    case 0x6000 ... 0x7fff: return rom.read(romBank[3] << 13 | (n13)address);
    case 0x8000 ... 0x9fff: return rom.read(romBank[4] << 13 | (n13)address);
    case 0xa000 ... 0xbfff: return rom.read(romBank[5] << 13 | (n13)address);
    }
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    switch(address) {
    case 0x4000: romBank[2] = data; return;
    case 0x6000: romBank[3] = data; return;
    case 0x8000: romBank[4] = data; return;
    case 0xa000: romBank[5] = data; return;
    case 0xfffe: romBank[2] = data << 1 | 0; romBank[3] = data << 1 | 1; return;
    case 0xffff: romBank[4] = data << 1 | 0; romBank[5] = data << 1 | 1; return;
    }
  }

  auto power() -> void override {
    romBank[0] = 0;
    romBank[1] = 1;
    romBank[2] = 2;
    romBank[3] = 3;
    romBank[4] = 4;
    romBank[5] = 5;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
  }

  n8 romBank[6];
};
