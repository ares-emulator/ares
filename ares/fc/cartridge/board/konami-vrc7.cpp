struct KonamiVRC7 : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;
  YM2413 ym2413;
  Node::Stream stream;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);

    stream = cartridge.node->append<Node::Stream>("YM2413");
    stream->setChannels(1);
    stream->setFrequency(uint(system.frequency() + 0.5) / cartridge.rate() / 36);
    stream->addLowPassFilter(2280.0, 1);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto main() -> void override {
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
      if(!disableFM) sample = ym2413.clock();
      stream->sample(sample);
    }

    tick();
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x6000) return cpu.mdr();
    if(address < 0x8000) return programRAM.read(address);

    uint8 bank;
    switch(address & 0xe000) {
    case 0x8000: bank = programBank[0]; break;
    case 0xa000: bank = programBank[1]; break;
    case 0xc000: bank = programBank[2]; break;
    case 0xe000: bank = 0xff; break;
    }
    address = bank << 13 | (uint13)address;
    return programROM.read(address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write(address, data);

    switch(address) {
    case 0x8000: programBank[0] = data; break;
    case 0x8010: programBank[1] = data; break;
    case 0x9000: programBank[2] = data; break;
    case 0x9010: ym2413.address(data); break;
    case 0x9030: ym2413.write(data); break;
    case 0xa000: characterBank[0] = data; break;
    case 0xa010: characterBank[1] = data; break;
    case 0xb000: characterBank[2] = data; break;
    case 0xb010: characterBank[3] = data; break;
    case 0xc000: characterBank[4] = data; break;
    case 0xc010: characterBank[5] = data; break;
    case 0xd000: characterBank[6] = data; break;
    case 0xd010: characterBank[7] = data; break;
    case 0xe000:
      if(disableFM && !data.bit(6)) ym2413.power(1);
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

  auto addressCIRAM(uint address) const -> uint {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical mirroring
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal mirroring
    case 2: return 0x0000 | address & 0x03ff;                 //one-screen mirroring (first)
    case 3: return 0x0400 | address & 0x03ff;                 //one-screen mirroring (second)
    }
    unreachable;
  }

  auto addressCHR(uint address) const -> uint {
    uint8 bank = characterBank[address >> 10 & 7];
    return bank << 10 | (uint10)address;
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(addressCHR(address));
    if(characterRAM) return characterRAM.read(addressCHR(address));
    return 0x00;
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(addressCHR(address), data);
  }

  auto power() -> void {
    ym2413.power();
    for(auto& bank : programBank) bank = 0;
    for(auto& bank : characterBank) bank = 0;
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
    programRAM.serialize(s);
    characterRAM.serialize(s);
    ym2413.serialize(s);
    s.array(programBank);
    s.array(characterBank);
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

  uint8 programBank[3];
  uint8 characterBank[8];
  uint2 mirror;
  uint1 disableFM;
  uint1 ramWritable;
  uint8 irqLatch;
  uint1 irqMode;
  uint1 irqEnable;
  uint1 irqAcknowledge;
  uint8 irqCounter;
  int16 irqScalar;
  uint1 irqLine;
  uint6 divider;
};
