struct Atari16k : Interface {
  Atari16k(Cartridge& cartridge, bool hasSaraRam = false) : Interface(cartridge), saraRam(hasSaraRam) {}
  Memory::Readable<n8> rom;
  SaraRam saraRam;
  n2 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address == 0x1ff6) bank = 0;
    if(address == 0x1ff7) bank = 1;
    if(address == 0x1ff8) bank = 2;
    if(address == 0x1ff9) bank = 3;

    if(address.bit(12)) {
      if(saraRam.readable(address)) return saraRam.read(address, data);
      return rom.read((bank * 0x1000) + (address & 0xfff));
    }

    return data;
  }
   
  auto write(n16 address, n8 data) -> n8 override {
    if(address == 0x1ff6) bank = 0;
    if(address == 0x1ff7) bank = 1;
    if(address == 0x1ff8) bank = 2;
    if(address == 0x1ff9) bank = 3;

    saraRam.write(address, data);
    return data;
  }

  auto power(bool reset) -> void override {
    bank = 0;
    saraRam.power();
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    saraRam.serialize(s);
  }
};
