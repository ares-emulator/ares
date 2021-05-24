struct MSX : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;

  MSX(Cartridge& cartridge, bool nemesis) : Interface(cartridge), nemesis(nemesis) {
  }

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    switch(address) {
    case 0x0000 ... 0x1fff: return rom.read(romBankFixed[0] << 13 | (n13)address);
    case 0x2000 ... 0x3fff: return rom.read(romBankFixed[1] << 13 | (n13)address);
    case 0x4000 ... 0x5fff: return rom.read(romBank[0] << 13 | (n13)address);
    case 0x6000 ... 0x7fff: return rom.read(romBank[1] << 13 | (n13)address);
    case 0x8000 ... 0x9fff: return rom.read(romBank[2] << 13 | (n13)address);
    case 0xa000 ... 0xbfff: return rom.read(romBank[3] << 13 | (n13)address);
    }
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    switch(address) {
    case 0x0000: romBank[2] = data; return;
    case 0x0001: romBank[3] = data; return;
    case 0x0002: romBank[0] = data; return;
    case 0x0003: romBank[1] = data; return;
    }
  }

  auto power() -> void override {
    romBankFixed[0] = 0 + (nemesis ? rom.size() / 8_KiB - 1 : 0);
    romBankFixed[1] = 1;
    romBank[0] = 0;
    romBank[1] = 1;
    romBank[2] = 2;
    romBank[3] = 3;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
  }

  const bool nemesis;
  n8 romBankFixed[2];
  n8 romBank[4];
};
