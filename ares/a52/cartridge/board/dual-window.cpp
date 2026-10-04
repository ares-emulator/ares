struct DualWindow40K : Interface {
  using Interface::Interface;

  auto load() -> bool override {
    return rom.load(pak, 40_KiB);
  }

  auto unload() -> void override {
    rom.reset();
  }

  auto power() -> void override {
    bank0 = 0;
    bank1 = 0;
  }

  auto read(n16 address, n8 data) -> n8 override {
    select(address);
    return peek(address, data);
  }

  auto peek(n16 address, n8 data) const -> n8 override {
    if(!rom) return data;
    if(address <= 0x0fff) return rom.read(bank0 * 0x1000 | (address & 0x0fff));
    if(address <= 0x1fff) return rom.read((4 + bank1) * 0x1000 | (address & 0x0fff));
    if(address < 0x4000) return data;
    return rom.read(0x8000 | (address & 0x1fff));
  }

  auto write(n16 address, n8 data) -> bool override {
    return select(address);
  }

  auto serialize(serializer& s) -> void override {
    s(bank0);
    s(bank1);
  }

private:
  auto select(n16 address) -> bool {
    if(address >= 0x0ff6 && address <= 0x0ff9) {
      bank0 = address - 0x0ff6;
      return true;
    }
    if(address >= 0x1ff6 && address <= 0x1ff9) {
      bank1 = address - 0x1ff6;
      return true;
    }
    return false;
  }

  ProgramROM rom;
  n2 bank0;
  n2 bank1;
};
