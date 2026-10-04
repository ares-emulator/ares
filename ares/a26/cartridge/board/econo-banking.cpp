struct EconoBanking : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n1 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    select(address);
    if(address.bit(12)) return rom.read(bank * 0x1000 + (address & 0x0fff));
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    select(address);
    return data;
  }

  auto power(bool reset) -> void override {
    bank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
  }

private:
  auto select(n16 address) -> void {
    if((address & 0x1840) == 0x0800) bank = 0;
    if((address & 0x1840) == 0x0840) bank = 1;
  }
};
