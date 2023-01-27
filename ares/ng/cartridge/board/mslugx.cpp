struct MSlugX : Interface {
  using Interface::Interface;

  Memory::Readable<n16> prom;
  Memory::Readable<n8 > mrom;
  Memory::Readable<n8 > crom;
  Memory::Readable<n8 > srom;
  Memory::Readable<n8 > vromA;
  Memory::Readable<n8 > vromB;

  auto load() -> void override {
    Interface::load(prom, "program.rom");
    Interface::load(mrom, "music.rom");;
    Interface::load(crom, "character.rom");
    Interface::load(srom, "static.rom");
    Interface::load(vromA, "voice-a.rom");
    Interface::load(vromB, "voice-b.rom");
  }

  auto unload() -> void override {
    prom.reset();
    mrom.reset();
    crom.reset();
    srom.reset();
    vromA.reset();
    vromB.reset();
  }

  auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(address <= 0x0fffff) return prom[address >> 1];
    if(address >= 0x2fffe0 && address <= 0x2fffef) {
      switch(command) {
        case 0x0001:
          data = (read8(0xdedd2 + ((counter >> 3) & 0xfff)) >> (~counter & 0x07)) & 1;
          counter++;
          return data;
        case 0x0fff:
          s32 select = read16(0x10f00a) - 1;
          return (read8(0xdedd2 + ((select >> 3) & 0xfff)) >> (~select & 0x07)) & 1;
      }

      return 0;
    }
    if(address >= 0x200000 && address <= 0x2fffff) {
      address = ((romBank + 1) * 0x100000) | n20(address);
      return prom[address >> 1];
    }
    return data;
  }

  auto read8(n24 address) -> n8 {
    if(address & 1) return cpu.read(0, 1, address & ~1, 0).byte(0);
    return cpu.read(1, 0, address & ~1, 0).byte(1);
  }

  auto read16(n24 address) -> n16 {
    return cpu.read(1, 1, address & ~1, 0);
  }

  auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(address >= 0x2fffe0 && address <= 0x2fffef) {
      switch(address & 0xf) {
        case 0x0: command = 0;           return;
        case 0x2:
        case 0x4: command |= data;       return;
        case 0xa: command = counter = 0; return;
     }
     return;
    }

    if(lower && address >= 0x2ffff0 && address <= 0x2fffff)  {
      romBank = data.bit(0, 2);
    }
  }

  auto readM(n32 address) -> n8 override {
    return mrom[address];
  }

  auto readC(n32 address) -> n8 override {
    return crom[address];
  }

  auto readS(n32 address) -> n8 override {
    return srom[address];
  }

  auto readVA(n32 address) -> n8 override {
    return vromA[address];
  }

  auto readVB(n32 address) -> n8 override {
    return vromB ? vromB[address] : vromA[address];
  }

  auto power() -> void override {
    romBank = 0;
    counter = 0;
    command = 0;
  }

  n8 romBank;
  u16 counter;
  u16 command;
};

