struct JockeyGP : Cmc50 {
  Memory::Writable<n16> ram;

  JockeyGP(Cartridge& cartridge) : Cmc50(cartridge, 0xac) {}

  auto load() -> void override {
    Cmc50::load();
    ram.allocate(0x100000 >> 1);
  }

  auto unload() -> void override {
    Cmc50::unload();
    ram.reset();
  }

  auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(address <= 0x0fffff) return prom[address >> 1];
    if(address >= 0x200000 && address <= 0x2fffff) return ram[address >> 1];
    return data;
  }

  auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(lower && address >= 0x200000 && address <= 0x2fffff) ram[address >> 1] = data;
  }

  auto power() -> void override {}

  auto serialize(serializer& s) -> void override {
    s(ram);
  }
};
