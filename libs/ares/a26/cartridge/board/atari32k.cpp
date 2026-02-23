struct Atari32k : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n3 bank;
  bool hasRam;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address) -> n8 override {
    if(address == 0x1ff4) bank = 0;
    if(address == 0x1ff5) bank = 1;
    if(address == 0x1ff6) bank = 2;
    if(address == 0x1ff7) bank = 3;
    if(address == 0x1ff8) bank = 4;
    if(address == 0x1ff9) bank = 5;
    if(address == 0x1ffa) bank = 6;
    if(address == 0x1ffb) bank = 7;

    if(address.bit(12)) {
      if(hasRam && address >= 0x1080 && address <= 0x10ff) return ram.read(address & 0x7f);
      return rom.read((bank * 0x1000) + (address & 0xfff));
    }

    return 0xff;
  }
   
  auto write(n16 address, n8 data) -> bool override {
    if(address == 0x1ff4) bank = 0;
    if(address == 0x1ff5) bank = 1;
    if(address == 0x1ff6) bank = 2;
    if(address == 0x1ff7) bank = 3;
    if(address == 0x1ff8) bank = 4;
    if(address == 0x1ff9) bank = 5;
    if(address == 0x1ffa) bank = 6;
    if(address == 0x1ffb) bank = 7;

    if(address >= 0x1000 && address <= 0x107f) {
      hasRam = true;
      ram.write(address & 0x7f, data);
      return true;
    }

    return false;
  }

  auto power() -> void override {
    bank = 0;
    hasRam = 0;
    ram.allocate(128);
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(hasRam);
    s(ram);
  }
};
