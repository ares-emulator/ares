struct Wickstead : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n3 arrangement;
  n3 pendingArrangement;
  u8 delay;
  bool pending;

  static constexpr u8 arrangements[8][4] = {
    {0, 0, 1, 3},
    {0, 1, 2, 3},
    {4, 5, 6, 7},
    {7, 4, 2, 3},
    {0, 0, 6, 7},
    {0, 1, 7, 6},
    {2, 3, 4, 5},
    {6, 0, 5, 1},
  };

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    advance(true);

    if(address >= 0x0030 && address <= 0x003f) {
      pendingArrangement = address & 7;
      delay = 3;
      pending = true;
      return data;
    }
    if(address >= 0x1000 && address <= 0x103f) return ram.read(address & 0x3f);
    if(address >= 0x1040 && address <= 0x107f) {
      ram.write(address & 0x3f, data);
      return data;
    }
    if(address.bit(12)) {
      auto segment = (address >> 10) & 3;
      auto chunk = arrangements[arrangement][segment];
      return rom.read(chunk * 1_KiB + (address & 0x3ff));
    }
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    advance(false);
    if(address >= 0x1040 && address <= 0x107f) {
      ram.write(address & 0x3f, data);
      return data;
    }
    return data;
  }

  auto power(bool reset) -> void override {
    arrangement = 0;
    pendingArrangement = 0;
    delay = 0;
    pending = false;
    ram.allocate(64, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    s(arrangement);
    s(pendingArrangement);
    s(delay);
    s(pending);
    s(ram);
  }

private:
  auto advance(bool isRead) -> void {
    if(!pending) return;
    if(delay) {
      delay--;
      return;
    }
    if(isRead) {
      arrangement = pendingArrangement;
      pending = false;
    }
  }
};
