struct Sunsoft5B : Interface {
  static auto create(string id) -> Interface* {
    if(id == "SUNSOFT-5B") return new Sunsoft5B;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;
  YM2149 ym2149;
  Node::Audio::Stream stream;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");

    stream = cartridge.node->append<Node::Audio::Stream>("YM2149");
    stream->setChannels(1);
    stream->setFrequency(u32(system.frequency() + 0.5) / cartridge.rate() / 16);
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
      stream->frame(output);
    }

    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;

    n8 bank;
    if((address & 0xe000) == 0x6000) bank = programBank[0];
    if((address & 0xe000) == 0x8000) bank = programBank[1];
    if((address & 0xe000) == 0xa000) bank = programBank[2];
    if((address & 0xe000) == 0xc000) bank = programBank[3];
    if((address & 0xe000) == 0xe000) bank = 0x3f;

    bool ramSelect = bank.bit(6);
    bool ramEnable = bank.bit(7);
    bank &= 0x3f;

    if(ramSelect) {
      if(!ramEnable) return data;
      return programRAM.read((n13)address);
    }

    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if((address & 0xe000) == 0x6000) {
      programRAM.write((n13)address, data);
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

  auto addressCHR(n32 address) -> n32 {
    n3 bank = address >> 10 & 7;
    return characterBank[bank] << 10 | (n10)address;
  }

  auto addressCIRAM(n32 address) -> n32 {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal
    case 2: return 0x0000 | address & 0x03ff;  //first
    case 3: return 0x0400 | address & 0x03ff;  //second
    }
    unreachable;
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
    ym2149.power();
    for(u32 level : range(32)) {
      volume[level] = 1.0 / pow(2, 1.0 / 2 * (31 - level));
    }
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(ym2149);
    s(port);
    s(programBank);
    s(characterBank);
    s(mirror);
    s(irqEnable);
    s(irqCounterEnable);
    s(irqCounter);
    s(divider);
  }

  n4  port;
  n8  programBank[4];
  n8  characterBank[8];
  n2  mirror;
  n1  irqEnable;
  n1  irqCounterEnable;
  n16 irqCounter;
  n4  divider;

//unserialized:
  f64 volume[32];
};
