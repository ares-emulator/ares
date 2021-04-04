struct SVP : Interface, SSP1601 {
  using Interface::Interface;
  Memory::Readable<n16> rom;
  Memory::Readable<n16> srom;
  Memory::Writable<n16> iram;
  Memory::Writable<n16> dram;

  struct Debugger {
    maybe<SVP&> super;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view type) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger;

  auto frequency() -> u32 override {
    return 23'000'000;
  }

  auto load() -> void override {
    Interface::load(rom,  "program.rom");
    Interface::load(srom, "svp.rom");
    iram.allocate(  2_KiB >> 1);
    dram.allocate(128_KiB >> 1);
    debugger.super = *this;
    debugger.load(cartridge->node);
  }

  auto unload() -> void override {
    debugger.unload(cartridge->node);
  }

  auto save() -> void override {
  }

  auto main() -> void override {
    if(auto vector = SSP1601::interrupt()) {
      switch(vector) {
      case 0xfffc: debugger.interrupt("RES");  break;
      case 0xfffd: debugger.interrupt("INT0"); break;
      case 0xfffe: debugger.interrupt("INT1"); break;
      case 0xffff: debugger.interrupt("INT2"); break;
      }
    }
    debugger.instruction();
    SSP1601::instruction();
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    if(address >= 0x000000 && address <= 0x1fffff) {
      data = rom[address >> 1];
    }
    if(address >= 0x300000 && address <= 0x37ffff) {
      data = dram[address >> 1];
    }
    if(address >= 0x390000 && address <= 0x39ffff) {
      address = (address & 0xe002) | ((address & 0x7c) << 6) | ((address & 0x1f80) >> 5);
      data = dram[address >> 1];
    }
    if(address >= 0x3a0000 && address <= 0x3affff) {
      address = (address & 0xf002) | ((address & 0x3c) << 6) | ((address & 0x0fc0) >> 4);
      data = dram[address >> 1];
    }
    if(address >= 0x3b0000 && address <= 0x3fffff) {
      data = 0xffff;
    }

    //VDP DMA from SVP to VDP VRAM responds with a one-access delay
    if(vdp.dma.active) swap(data, WRAM);
    return data;
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
    if(address >= 0x300000 && address <= 0x37ffff) {
      if(upper) dram[address >> 1].byte(1) = data.byte(1);
      if(lower) dram[address >> 1].byte(0) = data.byte(0);
    }
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(address == 0xa15000 || address == 0xa15002) {
      data = XST;
    }

    if(address == 0xa15004) {
      data = PM0;
      PM0 &= ~1;
    }

    return data;
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(address == 0xa15000 || address == 0xa15002) {
      XST = data;
      PM0 |= 2;
    }
  }

  auto vblank(bool line) -> void override {
  }

  auto hblank(bool line) -> void override {
    IRQ.bit(2) = line;  //INT1
  }

  auto power(bool reset) -> void override {
    SSP1601::power();
    iram.fill();
    dram.fill();
    PM0 = 0;
    PM1 = 0;
    PM2 = 0;
    XST = 0;
    PM4 = 0;
    PM5 = 0;
    PMC = 0;
    for(auto& word : PMAC[0]) word = 0;
    for(auto& word : PMAC[1]) word = 0;
    STATUS = 0;
    WRAM = 0;
  }

  auto serialize(serializer& s) -> void override {
    SSP1601::serialize(s);
    s(iram);
    s(dram);
    s(PM0);
    s(PM1);
    s(PM2);
    s(XST);
    s(PM4);
    s(PM5);
    s(PMAC[0]);
    s(PMAC[1]);
    s(STATUS);
    s(WRAM);
  }

  //SSP1601

  auto read(u16 address) -> u16 override {
    if(address <= 0x03ff) return step(1), iram[address];
    if(address >= 0xfc00) return step(1), srom[address];
    return step(4), rom[address];
  }

  auto getIncrement(u16 mode) -> s32 {
    s32 increment = n3(mode >> 11);
    if(increment == 0) return 0;
    if(increment != 7) increment--;
    increment = 1 << increment;
    if(mode & 0x8000) increment = -increment;
    return increment;
  }

  auto overwrite(n16& d, u16 s) -> void {
    if(s & 0xf000) d = (d & ~0xf000) | (s & 0xf000);
    if(s & 0x0f00) d = (d & ~0x0f00) | (s & 0x0f00);
    if(s & 0x00f0) d = (d & ~0x00f0) | (s & 0x00f0);
    if(s & 0x000f) d = (d & ~0x000f) | (s & 0x000f);
  }

  auto readPM(u8 reg, u16 d = 0) -> u32 {
    if(STATUS & SSP_PMC_SET) {
      PMAC[0][reg] = PMC;
      STATUS &= ~SSP_PMC_SET;
      return 0;
    }
    if(STATUS & SSP_PMC_HAVE_ADDRESS) {
      STATUS &= ~SSP_PMC_HAVE_ADDRESS;
    }
    if(reg == 4 || (ST & 0x60)) {
      u16 address = PMAC[0][reg] >>  0;
      u16 mode    = PMAC[0][reg] >> 16;
      if((mode & 0xfff0) == 0x0800) {
        PMAC[0][reg]++;
        d = rom[address | (n4(mode) << 16)];
      } else if((mode & 0x47ff) == 0x0018) {
        auto increment = getIncrement(mode);
        d = dram[address];
        PMAC[0][reg] += increment;
      } else {
        d = 0;
      }
      PMC = PMAC[0][reg];
      return d;
    }
    return -1;
  }

  auto writePM(u8 reg, u16 d) -> u32 {
    if(STATUS & SSP_PMC_SET) {
      PMAC[1][reg] = PMC;
      STATUS &= ~SSP_PMC_SET;
      return 0;
    }
    if(STATUS & SSP_PMC_HAVE_ADDRESS) {
      STATUS &= ~SSP_PMC_HAVE_ADDRESS;
    }
    if(reg == 4 || (ST & 0x60)) {
      u16 address = PMAC[1][reg] >>  0;
      u16 mode    = PMAC[1][reg] >> 16;
      if((mode & 0x43ff) == 0x0018) {
        auto increment = getIncrement(mode);
        if(mode & 0x0400) {
          overwrite(dram[address], d);
        } else {
          dram[address] = d;
        }
        PMAC[1][reg] += increment;
      } else if((mode & 0xfbff) == 0x4018) {
        if(mode & 0x0400) {
          overwrite(dram[address], d);
        } else {
          dram[address] = d;
        }
        PMAC[1][reg] += (address & 1) ? 31 : 1;
      } else if((mode & 0x47ff) == 0x001c) {
        auto increment = getIncrement(mode);
        iram[address] = d;
        PMAC[1][reg] += increment;
      }
      PMC = PMAC[1][reg];
      return d;
    }
    return -1;
  }

  auto readEXT(n3 r) -> u16 override {
    //PM0
    if(r == 0) {
      u32 d = readPM(0);
      if(d != -1) return d;
      d = PM0;
      PM0 &= ~2;
      return d;
    }

    //PM1
    if(r == 1) {
      u32 d = readPM(1);
      if(d != -1) return d;
      return PM1;
    }

    //PM2
    if(r == 2) {
      u32 d = readPM(2);
      if(d != -1) return d;
      return PM2;
    }

    //XST
    if(r == 3) {
      u32 d = readPM(3);
      if(d != -1) return d;
      return XST;
    }

    //PM4
    if(r == 4) {
      u32 d = readPM(4);
      if(d != -1) return d;
      return PM4;
    }

    //PMC
    if(r == 6) {
      if(STATUS & SSP_PMC_HAVE_ADDRESS) {
        STATUS |= SSP_PMC_SET;
        STATUS &=~SSP_PMC_HAVE_ADDRESS;
        return u16(PMC << 4 & 0xfff0) | u16(PMC >> 4 & 0x000f);
      } else {
        STATUS |= SSP_PMC_HAVE_ADDRESS;
        return u16(PMC);
      }
    }

    //AL
    if(r == 7) {
      STATUS &= ~SSP_PMC_HAVE_ADDRESS;
      STATUS &= ~SSP_PMC_SET;
    }

    return 0;
  }

  auto writeEXT(n3 r, u16 d) -> void override {
    //PM0
    if(r == 0) {
      u32 r = writePM(0, d);
      if(r != -1) return;
      PM0 = d;
    }

    //PM1
    if(r == 1) {
      u32 r = writePM(1, d);
      if(r != -1) return;
      PM1 = d;
    }

    //PM2
    if(r == 2) {
      u32 r = writePM(2, d);
      if(r != -1) return;
      PM2 = d;
    }

    //XST
    if(r == 3) {
      u32 r = writePM(3, d);
      if(r != -1) return;
      PM0 |= 1;
      XST = d;
    }

    //PM4
    if(r == 4) {
      u32 r = writePM(4, d);
      if(r != -1) return;
      PM4 = d;
    }

    //PMC
    if(r == 6) {
      if(STATUS & SSP_PMC_HAVE_ADDRESS) {
        STATUS |= SSP_PMC_SET;
        STATUS &=~SSP_PMC_HAVE_ADDRESS;
        PMC.bit(16,31) = d;
      } else {
        STATUS |= SSP_PMC_HAVE_ADDRESS;
        PMC.bit( 0,15) = d;
      }
    }
  }

  n16 PM0;
  n16 PM1;
  n16 PM2;
  n16 XST;
  n16 PM4;
  n16 PM5;
  n32 PMC;
  n32 PMAC[2][6];
  n32 STATUS;
  n16 WRAM;

  static constexpr u32 SSP_PMC_HAVE_ADDRESS = 0x0001;
  static constexpr u32 SSP_PMC_SET          = 0x0002;
};
