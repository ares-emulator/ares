//Mega Duck MD20S: MD2's ROM scheme (register 0x0001, write 1-7 selects a
//16KB page at 0x4000-0x7fff, 0x0000-0x3fff hardwired to page 0) plus MD0's
//SRAM half at 0x1000 (upper nibble, mask 0x30, 8KB pages at 0xa000-0xbfff,
//no enable line) for a plugged-in memory cart. Games: QR-Paint, Workboy
//(ROM patch).

struct MegaDuck20S : Interface {
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
    if(address >= 0x0000 && address <= 0x3fff) {
      return romRead(address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return romRead(bank.rom * 0x4000 + (address & 0x3fff));
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram) return 0xff;
      return ram.read(bank.ram * 0x2000 + (address & 0x1fff));
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address == 0x0001) {
      bank.rom = data % highPages();
      return;
    }

    if(address == 0x1000) {
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
    bank.rom = 1 % highPages();
    bank.ram = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(bank.rom);
    s(bank.ram);
  }

private:
  auto highPages() const -> u32 { return max(1, rom.size() / 0x4000); }
  auto ramPages()  const -> u32 { return max(1, ram.size() / 0x2000); }

  auto romRead(u32 offset) -> n8 {
    if(offset >= rom.size()) return 0xff;
    return rom.read(offset);
  }

  struct Bank {
    n8 rom = 1;
    n8 ram = 0;
  } bank;
};
