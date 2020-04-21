struct KonamiVRC6 : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;
  Node::Stream stream;

  struct Pulse {
    auto clock() -> void {
      if(--divider == 0) {
        divider = frequency + 1;
        cycle++;
        output = (mode == 1 || cycle > duty) ? volume : (uint4)0;
      }
      if(!enable) output = 0;
    }

    auto serialize(serializer& s) -> void {
      s.integer(mode);
      s.integer(duty);
      s.integer(volume);
      s.integer(enable);
      s.integer(frequency);
      s.integer(divider);
      s.integer(cycle);
      s.integer(output);
    }

     uint1 mode;
     uint3 duty;
     uint4 volume;
     uint1 enable;
    uint12 frequency;
    uint12 divider = 1;
     uint4 cycle;
     uint4 output;
  };

  struct Sawtooth {
    auto clock() -> void {
      if(--divider == 0) {
        divider = frequency + 1;
        if(++phase == 0) {
          accumulator += rate;
          if(++stage == 7) {
            stage = 0;
            accumulator = 0;
          }
        }
      }
      output = accumulator >> 3;
      if(!enable) output = 0;
    }

    auto serialize(serializer& s) -> void {
      s.integer(rate);
      s.integer(enable);
      s.integer(frequency);
      s.integer(divider);
      s.integer(phase);
      s.integer(stage);
      s.integer(accumulator);
      s.integer(output);
    }

     uint6 rate;
     uint1 enable;
    uint12 frequency;
    uint12 divider = 1;
     uint1 phase;
     uint3 stage;
     uint8 accumulator;
     uint5 output;
  };

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
    pinA0 = 1 << board["chip(type=VRC6)/pinout/a0"].natural();
    pinA1 = 1 << board["chip(type=VRC6)/pinout/a1"].natural();

    stream = cartridge.node->append<Node::Stream>("VRC6");
    stream->setChannels(1);
    stream->setFrequency(uint(system.frequency() + 0.5) / cartridge.rate());
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

    pulse1.clock();
    pulse2.clock();
    sawtooth.clock();
    double output = (pulse1.output + pulse2.output + sawtooth.output) / 61.0 * 0.25;
    stream->sample(-output);

    tick();
  }

  auto addressPRG(uint address) const -> uint {
    if((address & 0xc000) == 0x8000) return programBank[0] << 14 | (uint14)address;
    if((address & 0xe000) == 0xc000) return programBank[1] << 13 | (uint13)address;
    if((address & 0xe000) == 0xe000) return 0xff << 13 | (uint13)address;
    return 0;
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x6000) return cpu.mdr();
    if(address < 0x8000) return programRAM.read((uint13)address);
    return programROM.read(addressPRG(address));
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write((uint13)address, data);

    bool a0 = address & pinA0;
    bool a1 = address & pinA1;
    address = address & 0xf000 | a0 << 0 | a1 << 1;

    switch(address) {
    case 0x8000: case 0x8001: case 0x8002: case 0x8003:
      programBank[0] = data;
      break;
    case 0x9000:
      pulse1.volume = data.bit(0,3);
      pulse1.duty = data.bit(4,6);
      pulse1.mode = data.bit(7);
      break;
    case 0x9001:
      pulse1.frequency.bit(0,7) = data.bit(0,7);
      break;
    case 0x9002:
      pulse1.frequency.bit(8,11) = data.bit(0,3);
      pulse1.enable = data.bit(7);
      break;
    case 0xa000:
      pulse2.volume = data.bit(0,3);
      pulse2.duty = data.bit(4,6);
      pulse2.mode = data.bit(7);
      break;
    case 0xa001:
      pulse2.frequency.bit(0,7) = data.bit(0,7);
      break;
    case 0xa002:
      pulse2.frequency.bit(8,11) = data.bit(0,3);
      pulse2.enable = data.bit(7);
      break;
    case 0xb000:
      sawtooth.rate = data.bit(0,5);
      break;
    case 0xb001:
      sawtooth.frequency.bit(0,7) = data.bit(0,7);
      break;
    case 0xb002:
      sawtooth.frequency.bit(8,11) = data.bit(0,3);
      sawtooth.enable = data.bit(7);
      break;
    case 0xb003:
      mirror = data.bit(2,3);
      break;
    case 0xc000: case 0xc001: case 0xc002: case 0xc003:
      programBank[1] = data;
      break;
    case 0xd000: characterBank[0] = data; break;
    case 0xd001: characterBank[1] = data; break;
    case 0xd002: characterBank[2] = data; break;
    case 0xd003: characterBank[3] = data; break;
    case 0xe000: characterBank[4] = data; break;
    case 0xe001: characterBank[5] = data; break;
    case 0xe002: characterBank[6] = data; break;
    case 0xe003: characterBank[7] = data; break;
    case 0xf000:
      irqLatch = data;
      break;
    case 0xf001:
      irqAcknowledge = data.bit(0);
      irqEnable = data.bit(1);
      irqMode = data.bit(2);
      if(irqEnable) {
        irqCounter = irqLatch;
        irqScalar = 341;
      }
      irqLine = 0;
      break;
    case 0xf002:
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
    pulse1 = {};
    pulse2 = {};
    sawtooth = {};
    for(auto& bank : programBank) bank = 0;
    for(auto& bank : characterBank) bank = 0;
    mirror = 0;
    irqLatch = 0;
    irqMode = 0;
    irqEnable = 0;
    irqAcknowledge = 0;
    irqCounter = 0;
    irqScalar = 0;
    irqLine = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    characterRAM.serialize(s);
    pulse1.serialize(s);
    pulse2.serialize(s);
    sawtooth.serialize(s);
    s.integer(pinA0);
    s.integer(pinA1);
    s.array(programBank);
    s.array(characterBank);
    s.integer(mirror);
    s.integer(irqLatch);
    s.integer(irqMode);
    s.integer(irqEnable);
    s.integer(irqAcknowledge);
    s.integer(irqCounter);
    s.integer(irqScalar);
    s.integer(irqLine);
  }

  Pulse pulse1;
  Pulse pulse2;
  Sawtooth sawtooth;
  uint8 pinA0;
  uint8 pinA1;
  uint8 programBank[2];
  uint8 characterBank[8];
  uint2 mirror;
  uint8 irqLatch;
  uint1 irqMode;
  uint1 irqEnable;
  uint1 irqAcknowledge;
  uint8 irqCounter;
  int16 irqScalar;
  uint1 irqLine;
};
