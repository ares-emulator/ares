struct KonamiVRC6 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "KONAMI-VRC-6") return new KonamiVRC6;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;
  Node::Audio::Stream stream;

  struct Pulse {
    auto clock() -> void {
      if(--divider == 0) {
        divider = frequency + 1;
        cycle++;
        output = (mode == 1 || cycle > duty) ? volume : (n4)0;
      }
      if(!enable) output = 0;
    }

    auto serialize(serializer& s) -> void {
      s(mode);
      s(duty);
      s(volume);
      s(enable);
      s(frequency);
      s(divider);
      s(cycle);
      s(output);
    }

    n1  mode;
    n3  duty;
    n4  volume;
    n1  enable;
    n12 frequency;
    n12 divider;
    n4  cycle;
    n4  output;
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
      s(rate);
      s(enable);
      s(frequency);
      s(divider);
      s(phase);
      s(stage);
      s(accumulator);
      s(output);
    }

    n6  rate;
    n1  enable;
    n12 frequency;
    n12 divider;
    n1  phase;
    n3  stage;
    n8  accumulator;
    n5  output;
  };

  using Interface::Interface;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    pinA0 = 1 << pak->attribute("pinout/a0").natural();
    pinA1 = 1 << pak->attribute("pinout/a1").natural();

    stream = cartridge.node->append<Node::Audio::Stream>("VRC6");
    stream->setChannels(1);
    stream->setFrequency(u32(system.frequency() + 0.5) / cartridge.rate());
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
  }

  auto unload() -> void override {
    cartridge.node->remove(stream);
    stream.reset();
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
    f64 output = (pulse1.output + pulse2.output + sawtooth.output) / 61.0 * 0.25;
    stream->frame(-output);

    tick();
  }

  auto addressPRG(n32 address) const -> n32 {
    if((address & 0xc000) == 0x8000) return programBank[0] << 14 | (n14)address;
    if((address & 0xe000) == 0xc000) return programBank[1] << 13 | (n13)address;
    if((address & 0xe000) == 0xe000) return 0xff << 13 | (n13)address;
    return 0;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) return programRAM.read((n13)address);
    return programROM.read(addressPRG(address));
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write((n13)address, data);

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

  auto addressCIRAM(n32 address) const -> n32 {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical mirroring
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal mirroring
    case 2: return 0x0000 | address & 0x03ff;                 //one-screen mirroring (first)
    case 3: return 0x0400 | address & 0x03ff;                 //one-screen mirroring (second)
    }
    unreachable;
  }

  auto addressCHR(n32 address) const -> n32 {
    n8 bank = characterBank[address >> 10 & 7];
    return bank << 10 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(addressCHR(address));
    if(characterRAM) return characterRAM.read(addressCHR(address));
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(addressCHR(address), data);
  }

  auto power() -> void override {
    pulse1.divider = 1;
    pulse2.divider = 1;
    sawtooth.divider = 1;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(pulse1);
    s(pulse2);
    s(sawtooth);
    s(programBank);
    s(characterBank);
    s(mirror);
    s(irqLatch);
    s(irqMode);
    s(irqEnable);
    s(irqAcknowledge);
    s(irqCounter);
    s(irqScalar);
    s(irqLine);
  }

  Pulse pulse1;
  Pulse pulse2;
  Sawtooth sawtooth;
  n8  programBank[2];
  n8  characterBank[8];
  n2  mirror;
  n8  irqLatch;
  n1  irqMode;
  n1  irqEnable;
  n1  irqAcknowledge;
  n8  irqCounter;
  i16 irqScalar;
  n1  irqLine;

//unserialized:
  n8  pinA0;
  n8  pinA1;
};
