struct Tigervision : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n8 bank;
  n8 nextBank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address) -> n8 override {
    if(address.bit(12)) {
      if(bank != nextBank) bank = nextBank;
      if(address <= 0x17ff) return rom.read((bank * 0x800) + (address & 0x7ff));
      return rom.read((rom.size() - 0x800) + (address & 0x7ff));
    }

    return 0xff;
  }
   
  auto write(n16 address, n8 data) -> bool override {
    if(address.bit(12) && bank != nextBank) bank = nextBank;
    if(address <= 0x3f) nextBank = data;
    return false;
  }

  auto power() -> void override {
    bank = 0;
    nextBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(nextBank);
  }
};
