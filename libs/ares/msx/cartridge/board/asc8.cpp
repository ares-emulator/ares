//ASCII 8kbit

struct ASC8 : Interface {
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
    if(address >= 0x4000 && address <= 0x5fff) data = rom.read(bank[0] << 13 | (n13)address);
    if(address >= 0x6000 && address <= 0x7fff) data = rom.read(bank[1] << 13 | (n13)address);
    if(address >= 0x8000 && address <= 0x9fff) data = rom.read(bank[2] << 13 | (n13)address);
    if(address >= 0xa000 && address <= 0xbfff) data = rom.read(bank[3] << 13 | (n13)address);

    if(address >= 0xc000 && address <= 0xdfff) data = rom.read(bank[0] << 13 | (n13)address);
    if(address >= 0xe000 && address <= 0xffff) data = rom.read(bank[1] << 13 | (n13)address);
    if(address >= 0x0000 && address <= 0x1fff) data = rom.read(bank[2] << 13 | (n13)address);
    if(address >= 0x2000 && address <= 0x3fff) data = rom.read(bank[3] << 13 | (n13)address);

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x6000 && address <= 0x67ff) bank[0] = data;
    if(address >= 0x6800 && address <= 0x6fff) bank[1] = data;
    if(address >= 0x7000 && address <= 0x77ff) bank[2] = data;
    if(address >= 0x7800 && address <= 0x7fff) bank[3] = data;
  }

  auto power() -> void override {
    bank[0] = 0;
    bank[1] = 0;
    bank[2] = 0;
    bank[3] = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

  n8 bank[4];
};
