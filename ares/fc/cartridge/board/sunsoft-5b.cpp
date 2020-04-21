struct Sunsoft5B : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;
  YM2149 ym2149;
  Node::Stream stream;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);

    stream = cartridge.node->append<Node::Stream>("YM2149");
    stream->setChannels(1);
    stream->setFrequency(uint(system.frequency() + 0.5) / cartridge.rate() / 16);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto main() -> void {
    if(irqCounterEnable) {
      if(--irqCounter == 0xffff) {
        cpu.irqLine(irqEnable);
      }
    }

    if(!++divider) {
      auto channels = ym2149.clock();
      double output = 0.0;
      output += volume[channels[0]];
      output += volume[channels[1]];
      output += volume[channels[2]];
      stream->sample(output);
    }

    tick();
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x6000) return cpu.mdr();

    uint8 bank;
    if((address & 0xe000) == 0x6000) bank = programBank[0];
    if((address & 0xe000) == 0x8000) bank = programBank[1];
    if((address & 0xe000) == 0xa000) bank = programBank[2];
    if((address & 0xe000) == 0xc000) bank = programBank[3];
    if((address & 0xe000) == 0xe000) bank = 0x3f;

    bool ramSelect = bank.bit(6);
    bool ramEnable = bank.bit(7);
    bank &= 0x3f;

    if(ramSelect) {
      if(!ramEnable) return cpu.mdr();
      return programRAM.read((uint13)address);
    }

    address = bank << 13 | (uint13)address;
    return programROM.read(address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if((address & 0xe000) == 0x6000) {
      programRAM.write((uint13)address, data);
    }

    if(address == 0x8000) {
      port = data.bit(0,3);
    }

    if(address == 0xa000) {
      switch(port) {
      case 0x0: characterBank[0] = data; break;
      case 0x1: characterBank[1] = data; break;
      case 0x2: characterBank[2] = data; break;
      case 0x3: characterBank[3] = data; break;
      case 0x4: characterBank[4] = data; break;
      case 0x5: characterBank[5] = data; break;
      case 0x6: characterBank[6] = data; break;
      case 0x7: characterBank[7] = data; break;
      case 0x8: programBank[0] = data; break;
      case 0x9: programBank[1] = data; break;
      case 0xa: programBank[2] = data; break;
      case 0xb: programBank[3] = data; break;
      case 0xc: mirror = data.bit(0,1); break;
      case 0xd:
        irqCounterEnable = data.bit(0);
        irqEnable = data.bit(7);
        if(irqEnable == 0) cpu.irqLine(0);
        break;
      case 0xe: irqCounter.byte(0) = data; break;
      case 0xf: irqCounter.byte(1) = data; break;
      }
    }

    if(address == 0xc000) {
      ym2149.select(data);
    }

    if(address == 0xe000) {
      ym2149.write(data);
    }
  }

  auto addressCHR(uint address) -> uint {
    uint3 bank = address >> 10 & 7;
    return characterBank[bank] << 10 | (uint10)address;
  }

  auto addressCIRAM(uint address) -> uint {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal
    case 2: return 0x0000 | address & 0x03ff;  //first
    case 3: return 0x0400 | address & 0x03ff;  //second
    }
    unreachable;
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
    ym2149.power();

    port = 0;
    for(auto& bank : programBank) bank = 0;
    for(auto& bank : characterBank) bank = 0;
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
    programRAM.serialize(s);
    characterRAM.serialize(s);
    ym2149.serialize(s);
    s.integer(port);
    s.array(programBank);
    s.array(characterBank);
    s.integer(mirror);
    s.integer(irqEnable);
    s.integer(irqCounterEnable);
    s.integer(irqCounter);
    s.integer(divider);
  }

   uint4 port;
   uint8 programBank[4];
   uint8 characterBank[8];
   uint2 mirror;
   uint1 irqEnable;
   uint1 irqCounterEnable;
  uint16 irqCounter;
   uint4 divider;

//unserialized:
  double volume[32];
};
