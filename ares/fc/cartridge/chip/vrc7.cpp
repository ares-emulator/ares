//Konami VRC7

struct VRC7 : YM2413, Chip {
  Node::Stream stream;

  VRC7(Board& board) : Chip(board) {
    stream = Node::append<Node::Stream>(cartridge.node, {}, "Stream");
    stream->setChannels(1);
    stream->setFrequency(uint(system.frequency() + 0.5) / cartridge.rate() / 36);
    stream->addLowPassFilter(2280.0, 1);
  }

  ~VRC7() {
    stream = {};
  }

  auto main() -> void {
    if(irqEnable) {
      if(irqMode == 0) {
        irqScalar -= 3;
        if(irqScalar <= 0) {
          irqScalar += 341;
          if(irqCounter == 0xff) {
            irqCounter = irqLatch;
            irqLine = 1;
          } else {
            irqCounter++;
          }
        }
      }

      if(irqMode == 1) {
        if(irqCounter == 0xff) {
          irqCounter = irqLatch;
          irqLine = 1;
        } else {
          irqCounter++;
        }
      }
    }
    cpu.irqLine(irqLine);

    if(++divider == 36) {
      divider = 0;
      double sample = 0.0;
      if(!disableFM) sample = YM2413::clock();
      stream->sample(sample);
    }

    tick();
  }

  auto writeIO(uint addr, uint8 data) -> void {
    switch(addr) {
    case 0x8000: prgBank[0] = data; break;
    case 0x8010: prgBank[1] = data; break;
    case 0x9000: prgBank[2] = data; break;
    case 0x9010: YM2413::address(data); break;
    case 0x9030: YM2413::write(data); break;
    case 0xa000: chrBank[0] = data; break;
    case 0xa010: chrBank[1] = data; break;
    case 0xb000: chrBank[2] = data; break;
    case 0xb010: chrBank[3] = data; break;
    case 0xc000: chrBank[4] = data; break;
    case 0xc010: chrBank[5] = data; break;
    case 0xd000: chrBank[6] = data; break;
    case 0xd010: chrBank[7] = data; break;
    case 0xe000:
      if(disableFM && !data.bit(6)) YM2413::power(1);
      mirror = data.bit(0,1);
      disableFM = data.bit(6);
      ramWritable = data.bit(7);
      break;

    case 0xe010:
      irqLatch = data;
      break;

    case 0xf000:
      irqAcknowledge = data.bit(0);
      irqEnable = data.bit(1);
      irqMode = data.bit(2);
      if(irqEnable) {
        irqCounter = irqLatch;
        irqScalar = 341;
      }
      irqLine = 0;
      break;

    case 0xf010:
      irqEnable = irqAcknowledge;
      irqLine = 0;
      break;
    }
  }

  auto addrPRG(uint addr) const -> uint {
    uint bank = 0;
    switch(addr & 0xe000) {
    case 0x8000: bank = prgBank[0]; break;
    case 0xa000: bank = prgBank[1]; break;
    case 0xc000: bank = prgBank[2]; break;
    case 0xe000: bank = 0xff; break;
    }
    return (bank * 0x2000) + (addr & 0x1fff);
  }

  auto addrCHR(uint addr) const -> uint {
    uint bank = chrBank[addr / 0x0400];
    return (bank * 0x0400) + (addr & 0x03ff);
  }

  auto addrCIRAM(uint addr) const -> uint {
    switch(mirror) {
    case 0: return ((addr & 0x0400) >> 0) | (addr & 0x03ff);  //vertical mirroring
    case 1: return ((addr & 0x0800) >> 1) | (addr & 0x03ff);  //horizontal mirroring
    case 2: return 0x0000 | (addr & 0x03ff);                  //one-screen mirroring (first)
    case 3: return 0x0400 | (addr & 0x03ff);                  //one-screen mirroring (second)
    }
    unreachable;
  }

  auto power() -> void {
    for(auto& n : prgBank) n = 0;
    for(auto& n : chrBank) n = 0;
    mirror = 0;
    disableFM = 1;
    ramWritable = 1;

    irqLatch = 0;
    irqMode = 0;
    irqEnable = 0;
    irqAcknowledge = 0;

    irqCounter = 0;
    irqScalar = 0;
    irqLine = 0;

    divider = 0;
  }

  auto serialize(serializer& s) -> void {
    YM2413::serialize(s);

    s.array(prgBank);
    s.array(chrBank);
    s.integer(mirror);
    s.integer(disableFM);
    s.integer(ramWritable);

    s.integer(irqLatch);
    s.integer(irqMode);
    s.integer(irqEnable);
    s.integer(irqAcknowledge);

    s.integer(irqCounter);
    s.integer(irqScalar);
    s.integer(irqLine);

    s.integer(divider);
  }

  uint8 prgBank[3];
  uint8 chrBank[8];
  uint2 mirror;
  uint1 disableFM;
  uint1 ramWritable;

  uint8 irqLatch;
  bool irqMode;
  bool irqEnable;
  bool irqAcknowledge;

  uint8 irqCounter;
  int irqScalar;
  bool irqLine;

  uint6 divider;
};
