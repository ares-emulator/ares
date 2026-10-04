struct MDM : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n7 bank;
  n1 bankingDisabled;

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
    bankingDisabled = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(bankingDisabled);
  }

private:
  auto bankswitch(n16 address) -> void {
    if(bankingDisabled || (address & 0x1c00) != 0x0800) return;
    auto selector = address & 0x00ff;
    bank = selector % (rom.size() >> 12);
    if(selector > 127) bankingDisabled = 1;
  }
};
