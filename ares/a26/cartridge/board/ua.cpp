struct UA8k : Interface {
  UA8k(Cartridge& cartridge, bool swapped = false) : Interface(cartridge), swapped(swapped) {}
  Memory::Readable<n8> rom;
  const bool swapped;
  n1 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    bankswitch(address);
    if(address.bit(12)) return rom.read((bank * 0x1000) + (address & 0x0fff));
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    bankswitch(address);
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
    if((address & 0x1260) == 0x0220) bank = swapped ? 1 : 0;
    if((address & 0x1260) == 0x0240) bank = swapped ? 0 : 1;
  }
};
