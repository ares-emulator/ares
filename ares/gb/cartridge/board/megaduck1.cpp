//Mega Duck MD1: register 0xb000, write 0-1 selects a 32KB page shown flat
//across the whole 0x0000-0x7fff window. No SRAM. Max 64KB ROM.

struct MegaDuck1 : Interface {
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
    if(address >= 0x0000 && address <= 0x7fff) {
      return romRead(bank * 0x8000 + (address & 0x7fff));
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address == 0xb000) {
      bank = data % romPages();
      return;
    }
  }

  auto power() -> void override {
    bank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

private:
  auto romPages() const -> u32 { return max(1, rom.size() / 0x8000); }

  auto romRead(u32 offset) -> n8 {
    if(offset >= rom.size()) return 0xff;
    return rom.read(offset);
  }

  n8 bank = 0;
};
