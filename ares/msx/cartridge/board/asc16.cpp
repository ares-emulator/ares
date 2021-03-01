//ASCII 16kbit

struct ASC16 : Interface {
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
    if(address >= 0x4000 && address <= 0x7fff) data = rom.read(bank[0] << 14 | (n14)address);
    if(address >= 0x8000 && address <= 0xbfff) data = rom.read(bank[1] << 14 | (n14)address);

    if(address >= 0xc000 && address <= 0xffff) data = rom.read(bank[0] << 14 | (n14)address);
    if(address >= 0x0000 && address <= 0x3fff) data = rom.read(bank[1] << 14 | (n14)address);

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x6000 && address <= 0x6fff) bank[0] = data;
    if(address >= 0x7000 && address <= 0xbfff) bank[1] = data;
  }

  auto power() -> void override {
    bank[0] = 0x0f;  //R-Type = 0x0f; others = 0x00
    bank[1] = 0x00;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

  n8 bank[2];
};
