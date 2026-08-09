struct Linear : Interface {
  enum class Wiring {
    Upper,
    Split,
    Full,
    Overlapping,
  };

  Linear(Cartridge& cartridge, u32 size, Wiring wiring) : Interface(cartridge), size(size), wiring(wiring) {}

  auto load() -> bool override {
    return rom.load(pak, size);
  }

  auto unload() -> void override {
    rom.reset();
  }

  auto read(n16 address, n8 data) -> n8 override {
    return peek(address, data);
  }

  auto peek(n16 address, n8 data) const -> n8 override {
    if(!rom) return data;

    switch(wiring) {
    case Wiring::Upper:
      if(address < 0x4000) return data;
      return rom.read(address & (size - 1));
    case Wiring::Split:
      if(address < 0x4000) return rom.read(address & 0x1fff);
      return rom.read(0x2000 | (address & 0x1fff));
    case Wiring::Full:
      return rom.read(address & (size - 1));
    case Wiring::Overlapping:
      //Map 12KiB at $7000-$9fff, then repeat the final 8KiB through $a000-$bfff.
      if(address < 0x3000) return data;
      if(address < 0x6000) return rom.read(address - 0x3000);
      return rom.read(0x2000 | (address & 0x1fff));
    }

    return data;
  }

private:
  ProgramROM rom;
  u32 size;
  Wiring wiring;
};
