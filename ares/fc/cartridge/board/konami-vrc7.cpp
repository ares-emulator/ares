struct KonamiVRC7 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "KONAMI-VRC-7") return new KonamiVRC7;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;
  YM2413 ym2413;
  Node::Audio::Stream stream;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");

    stream = cartridge.node->append<Node::Audio::Stream>("YM2413");
    stream->setChannels(1);
    stream->setFrequency(u32(system.frequency() + 0.5) / cartridge.rate() / 36);
    stream->addLowPassFilter(2280.0, 1);
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

    if(++divider == 36) {
      divider = 0;
      double sample = 0.0;
      if(!disableFM) sample = ym2413.clock();
      stream->frame(sample);
    }

    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) return programRAM.read(address);

    n8 bank;
    switch(address & 0xe000) {
    case 0x8000: bank = programBank[0]; break;
    case 0xa000: bank = programBank[1]; break;
    case 0xc000: bank = programBank[2]; break;
    case 0xe000: bank = 0xff; break;
    }
    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
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
    ym2413.power();
    disableFM = 1;
    ramWritable = 1;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(ym2413);
    s(programBank);
    s(characterBank);
    s(mirror);
    s(disableFM);
    s(ramWritable);
    s(irqLatch);
    s(irqMode);
    s(irqEnable);
    s(irqAcknowledge);
    s(irqCounter);
    s(irqScalar);
    s(irqLine);
    s(divider);
  }

  n8  programBank[3];
  n8  characterBank[8];
  n2  mirror;
  n1  disableFM;
  n1  ramWritable;
  n8  irqLatch;
  n1  irqMode;
  n1  irqEnable;
  n1  irqAcknowledge;
  n8  irqCounter;
  i16 irqScalar;
  n1  irqLine;
  n6  divider;
};
