//SUNSOFT-5B

struct Sunsoft5B : YM2149, Board {
  Node::Stream stream;

  Sunsoft5B(Markup::Node& document) : Board(document) {
    stream = Node::append<Node::Stream>(cartridge.node, {}, "Stream");
    stream->setChannels(1);
    stream->setFrequency(uint(system.frequency() + 0.5) / cartridge.rate() / 16);
  }

  ~Sunsoft5B() {
    stream = {};
  }

  auto main() -> void {
    if(irqCounterEnable) {
      if(--irqCounter == 0xffff) {
        cpu.irqLine(irqEnable);
      }
    }

    if(!++divider) {
      auto channels = YM2149::clock();
      double output = 0.0;
      output += volume[channels[0]];
      output += volume[channels[1]];
      output += volume[channels[2]];
      stream->sample(output);
    }

    tick();
  }

  auto readPRG(uint addr) -> uint8 {
    if(addr < 0x6000) return cpu.mdr();

    uint8 bank = 0x3f;  //((addr & 0xe000) == 0xe000
    if((addr & 0xe000) == 0x6000) bank = prgBank[0];
    if((addr & 0xe000) == 0x8000) bank = prgBank[1];
    if((addr & 0xe000) == 0xa000) bank = prgBank[2];
    if((addr & 0xe000) == 0xc000) bank = prgBank[3];

    bool ramEnable = bank & 0x80;
    bool ramSelect = bank & 0x40;
    bank &= 0x3f;

    if(ramSelect) {
      if(!ramEnable) return cpu.mdr();
      return prgram.data[addr & 0x1fff];
    }

    addr = (bank << 13) | (addr & 0x1fff);
    return prgrom.read(addr);
  }

  auto writePRG(uint addr, uint8 data) -> void {
    if((addr & 0xe000) == 0x6000) {
      prgram.data[addr & 0x1fff] = data;
    }

    if(addr == 0x8000) {
      port = data & 0x0f;
    }

    if(addr == 0xa000) {
      switch(port) {
      case  0: chrBank[0] = data; break;
      case  1: chrBank[1] = data; break;
      case  2: chrBank[2] = data; break;
      case  3: chrBank[3] = data; break;
      case  4: chrBank[4] = data; break;
      case  5: chrBank[5] = data; break;
      case  6: chrBank[6] = data; break;
      case  7: chrBank[7] = data; break;
      case  8: prgBank[0] = data; break;
      case  9: prgBank[1] = data; break;
      case 10: prgBank[2] = data; break;
      case 11: prgBank[3] = data; break;
      case 12: mirror = data & 3; break;
      case 13:
        irqEnable = data & 0x80;
        irqCounterEnable = data & 0x01;
        if(irqEnable == 0) cpu.irqLine(0);
        break;
      case 14: irqCounter = (irqCounter & 0xff00) | (data << 0); break;
      case 15: irqCounter = (irqCounter & 0x00ff) | (data << 8); break;
      }
    }

    if(addr == 0xc000) {
      YM2149::select(data);
    }

    if(addr == 0xe000) {
      YM2149::write(data);
    }
  }

  auto addrCHR(uint addr) -> uint {
    uint8 bank = (addr >> 10) & 7;
    return (chrBank[bank] << 10) | (addr & 0x03ff);
  }

  auto addrCIRAM(uint addr) -> uint {
    switch(mirror) {
    case 0: return ((addr & 0x0400) >> 0) | (addr & 0x03ff);  //vertical
    case 1: return ((addr & 0x0800) >> 1) | (addr & 0x03ff);  //horizontal
    case 2: return 0x0000 | (addr & 0x03ff);  //first
    case 3: return 0x0400 | (addr & 0x03ff);  //second
    }
    unreachable;
  }

  auto readCHR(uint addr) -> uint8 {
    if(addr & 0x2000) return ppu.readCIRAM(addrCIRAM(addr));
    return Board::readCHR(addrCHR(addr));
  }

  auto writeCHR(uint addr, uint8 data) -> void {
    if(addr & 0x2000) return ppu.writeCIRAM(addrCIRAM(addr), data);
    return Board::writeCHR(addrCHR(addr), data);
  }

  auto power() -> void {
    YM2149::power();

    port = 0;
    for(auto& n : prgBank) n = 0;
    for(auto& n : chrBank) n = 0;
    mirror = 0;
    irqEnable = 0;
    irqCounterEnable = 0;
    irqCounter = 0;

    divider = 0;
    for(uint level : range(32)) {
      volume[level] = 1.0 / pow(2, 1.0 / 2 * (31 - level));
    }
  }

  auto serialize(serializer& s) -> void {
    YM2149::serialize(s);
    Board::serialize(s);

    s.integer(port);
    s.array(prgBank);
    s.array(chrBank);
    s.integer(mirror);
    s.integer(irqEnable);
    s.integer(irqCounterEnable);
    s.integer(irqCounter);

    s.integer(divider);
  }

  uint4 port;
  uint8 prgBank[4];
  uint8 chrBank[8];
  uint2 mirror;
  bool irqEnable;
  bool irqCounterEnable;
  uint16 irqCounter;

  uint4 divider;
  double volume[32];
};
