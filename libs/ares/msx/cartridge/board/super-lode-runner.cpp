//Super Lode Runner
//(not working: requires MSX BASIC)

struct SuperLodeRunner : Interface {
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
    if(address >= 0x8000 && address <= 0xbfff) data = rom.read(bank << 14 | (n14)address);
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x3fff) bank = data;
  }

  auto power() -> void override {
    bank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

  n8 bank;
};
