//Mega Duck MD2: register 0x0001, write 1-7 selects a 16KB page at
//0x4000-0x7fff; the 16KB at 0x0000-0x3fff is hardwired to page 0. No SRAM.
//Max 256KB ROM. The most common cartridge type.
//
//note: Duck Adventures uses this board and drops sprites across the screen
//after its title screen. Banking, register translation and interrupts all
//check out; it also fails on MAME and SameDuck, so this is likely a bad
//dump rather than a board bug.

struct MegaDuck2 : Interface {
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
    if(address >= 0x0000 && address <= 0x3fff) {
      return romRead(address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return romRead(bank * 0x4000 + (address & 0x3fff));
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address == 0x0001) {
      bank = data % highPages();
      return;
    }
  }

  auto power() -> void override {
    bank = 1 % highPages();
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

private:
  auto highPages() const -> u32 { return max(1, rom.size() / 0x4000); }

  auto romRead(u32 offset) -> n8 {
    if(offset >= rom.size()) return 0xff;
    return rom.read(offset);
  }

  n8 bank = 1;
};
