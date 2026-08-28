struct Enhanced3F : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n8 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    if(!address.bit(11)) return rom.read(bank * 0x800 + (address & 0x07ff));
    return rom.read(rom.size() - 0x800 + (address & 0x07ff));
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(address <= 0x003f) bank = data % (rom.size() >> 11);
    return data;
  }

  auto power(bool reset) -> void override {
    //Stella powers bank 0; MAME powers the final bank.
    bank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }
};
