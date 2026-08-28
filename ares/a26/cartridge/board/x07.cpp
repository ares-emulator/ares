struct X07 : Interface {
  using Interface::Interface;

  Memory::Readable<n8> rom;
  n4 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    bankswitch(address);
    if(address.bit(12)) return rom.read(bank * 0x1000 + (address & 0x0fff));
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    bankswitch(address & 0x1fff);
    return data;
  }

  auto power(bool reset) -> void override {
    bank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

private:
  auto bankswitch(n16 address) -> void {
    if((address & 0x180f) == 0x080d) {
      bank = address.bit(4, 7);
    } else if((address & 0x1880) == 0 && bank >= 14) {
      bank = 14 | address.bit(6);
    }
  }
};
