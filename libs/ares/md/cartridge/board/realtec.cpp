struct Realtec : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(enable)
      address = (address & (0x20000<<size)-1) + bank * (0x20000<<size);
    else
      address = (address & 0x1fff) + 0x7e000;
    return address >> 1 < rom.size() ? rom[address >> 1] : data;
  }

  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(address == 0x400000 && upper) {
      bank.bit(2, 3) = data.bit(9, 10);
      enable = data.bit(8); // unconfirmed function
    }
    if(address == 0x402000 && upper)
      size = data.bit(9, 10); // exact function is speculative; known carts write either 02 (if 2 256KB rom banks) or 04 (if 1 512KB rom bank)
    if(address == 0x404000 && upper)
      bank.bit(0, 1) = data.bit(9, 10);
  }

  auto power(bool reset) -> void override {
    bank   = 0;
    size   = 0;
    enable = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(size);
    s(enable);
  }

  n4 bank;
  n2 size;
  n1 enable;
};
