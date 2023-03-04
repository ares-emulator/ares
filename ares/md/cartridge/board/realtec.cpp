struct Realtec : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(address < size * 0x20000)
      address = address + bank * 0x20000;
    else
      address = (address & 0x1fff) + 0x7e000;
    return rom[address >> 1];
  }

  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(address == 0x400000 && upper)
      bank.bit(3, 4) = data.bit(9, 10);
    if(address == 0x402000 && upper)
      size = data.bit(8, 13);
    if(address == 0x404000 && upper)
      bank.bit(0, 2) = data.bit(8, 10);
  }

  auto power(bool reset) -> void override {
    bank = 0;
    size = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(size);
  }

  n5 bank;
  n5 size;
};
