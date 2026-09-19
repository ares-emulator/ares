//Mega Duck MD25S: MD2's ROM scheme (register 0x0001, write 1-7 selects a
//16KB page at 0x4000-0x7fff, 0x0000-0x3fff hardwired to page 0) plus
//MBC5-style SRAM: enable at 0x0000 (write 0x0a to enable, anything else
//disables), 8KB bank select at 0x4000 (0-15, so up to 128KB). Games:
//Pokemon Red/Blue (ROM patch).

struct MegaDuck25S : Interface {
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
      if(!ram || !io.ramEnable) return 0xff;
      return ram.read(bank.ram * 0x2000 + (address & 0x1fff));
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    //MD2's ROM bank register lives at the single address 0x0001, so SRAM
    //enable - also documented as a single address, unlike MBC5's full
    //0x0000-0x1fff range - is placed at 0x0000 to avoid colliding with it
    if(address == 0x0000) {
      io.ramEnable = data == 0x0a;
      return;
    }

    if(address == 0x0001) {
      bank.rom = data % highPages();
      return;
    }

    if(address == 0x4000) {
      bank.ram = data % ramPages();
      return;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(!ram || !io.ramEnable) return;
      ram.write(bank.ram * 0x2000 + (address & 0x1fff), data);
      return;
    }
  }

  auto power() -> void override {
    bank.rom = 1 % highPages();
    bank.ram = 0;
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(bank.rom);
    s(bank.ram);
    s(io.ramEnable);
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

  struct IO {
    n1 ramEnable;
  } io;
};
