struct Activision8k : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n1 bank;
  bool lastAccessWasFE;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address.bit(12)) data = rom.read((bank * 0x1000) + (address & 0xfff));
    access(address, data);
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    access(address, data);
    return data;
  }

  auto power(bool reset) -> void override {
    bank = 0;
    lastAccessWasFE = false;
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    s(lastAccessWasFE);
  }

private:
  auto access(n16 address, n8 data) -> void {
    if(lastAccessWasFE) {
      //Released 8 KiB FE cartridges ignore D7-D6 and invert D5 to select one of two banks.
      bank = data.bit(5) ^ 1;
      lastAccessWasFE = false;
      return;
    }
    lastAccessWasFE = address == 0x01fe;
  }
};
