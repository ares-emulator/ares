//Mega Duck cartridges larger than 32KB carry a mapper with two independent
//windows: 0x0000-0x3fff shows the first half of a 32KB page, and 0x4000-0x7fff
//shows any 16KB page.
//
//  write 0x0001  select the 16KB page at 0x4000-0x7fff
//  write 0xb000  select a 32KB page: the low window takes it, and the high
//                window takes its upper half, so the whole page reads flat
//
//at reset the two windows show pages 0 and 1, which is the first 32KB laid out
//flat, so a cartridge that never writes either register behaves like a plain
//linear board. MAME cannot tell from a dump whether the low window is fixed or
//banked and installs both handlers; the same is done here.

struct MegaDuck : Interface {
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
      return romRead(bank.low * 0x8000 + (address & 0x3fff));
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return romRead(bank.high * 0x4000 + (address & 0x3fff));
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address == 0x0001) {
      bank.high = data % highPages();
      return;
    }

    if(address == 0xb000) {
      bank.low  = data % lowPages();
      bank.high = (data * 2 + 1) % highPages();
      return;
    }
  }

  auto power() -> void override {
    bank.low  = 0;
    bank.high = 1 % highPages();
  }

  auto serialize(serializer& s) -> void override {
    s(bank.low);
    s(bank.high);
  }

private:
  //sizes are not always a power of two, so wrap by the page count rather than
  //masking, which is what MAME's map_non_power_of_two ends up doing
  auto lowPages() const -> u32 { return max(1, rom.size() / 0x8000); }
  auto highPages() const -> u32 { return max(1, rom.size() / 0x4000); }

  auto romRead(u32 offset) -> n8 {
    if(offset >= rom.size()) return 0xff;
    return rom.read(offset);
  }

  struct Bank {
    n8 low = 0;
    n8 high = 1;
  } bank;
};
