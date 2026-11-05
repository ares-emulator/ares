//Mega Duck MD0: register 0x1000, lower nibble (0-15) selects a 32KB page
//shown flat across the whole 0x0000-0x7fff window; upper nibble (mask 0x30)
//selects an 8KB SRAM page at 0xa000-0xbfff, no SRAM enable line. Max 512KB
//ROM, 32KB SRAM. Used by the laptop models but also by handheld homebrew
//(Duck Duck Wordyl), so it stays implemented despite no keyboard, RTC or
//speech support existing yet for the laptop's serial peripherals - laptop
//carts will bank-switch fine and then stall waiting on hardware ares does
//not expose.

struct MegaDuck0 : Interface {
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
    if(address >= 0x0000 && address <= 0x7fff) {
      return romRead(bank.rom * 0x8000 + (address & 0x7fff));
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return 0xff;
      return ram.read(bank.ram * 0x2000 + (address & 0x1fff));
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address == 0x1000) {
      bank.rom = data.bit(0,3) % romPages();
      bank.ram = data.bit(4,5) % ramPages();
      return;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return;
      ram.write(bank.ram * 0x2000 + (address & 0x1fff), data);
      return;
    }
  }

  auto power() -> void override {
    bank = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(bank.rom);
    s(bank.ram);
  }

private:
  auto romPages() const -> u32 { return max(1, rom.size() / 0x8000); }
  auto ramPages() const -> u32 { return max(1, ram.size() / 0x2000); }

  auto romRead(u32 offset) -> n8 {
    if(offset >= rom.size()) return 0xff;
    return rom.read(offset);
  }

  struct Bank {
    n8 rom = 0;
    n8 ram = 0;
  } bank;
};
