struct AmigaFC : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  u32 bankCount = 0;
  u8 activeBank = 0;
  u8 targetBank = 0;

  auto load() -> void override {
    if(!Interface::load(rom, "program.rom")) return;
    bankCount = rom.size() / 4_KiB;
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address == 0x1ffc) activeBank = targetBank % bankCount;
    if(address.bit(12)) return rom.read(activeBank * 0x1000 + (address & 0xfff));
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address == 0x1ff8) {
      targetBank = data & 3;
    } else if(address == 0x1ff9) {
      auto highBank = (u32)data << 2;
      if(highBank < bankCount) {
        targetBank += highBank;
        targetBank %= bankCount;
      } else {
        targetBank = data % bankCount;
      }
    } else if(address == 0x1ffc) {
      activeBank = targetBank % bankCount;
    }
    return data;
  }

  auto power(bool reset) -> void override {
    activeBank = 0;
    targetBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(activeBank);
    s(targetBank);
  }
};
