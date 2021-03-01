struct KonamiVRC4 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "KONAMI-VRC-4") return new KonamiVRC4;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    pinA0 = 1 << pak->attribute("pinout/a0").natural();
    pinA1 = 1 << pak->attribute("pinout/a1").natural();
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
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
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) return programRAM.read((n13)address);

    n5 bank, banks = programROM.size() >> 13;
    switch(address & 0xe000) {
    case 0x8000: bank = programMode == 0 ? programBank[0] : n5(banks - 2); break;
    case 0xa000: bank = programBank[1]; break;
    case 0xc000: bank = programMode == 1 ? programBank[0] : n5(banks - 2); break;
    case 0xe000: bank = banks - 1; break;
    }
    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write((n13)address, data);

    bool a0 = address & pinA0;
    bool a1 = address & pinA1;
    address &= 0xf000;
    address |= a0 << 0 | a1 << 1;

    switch(address) {
    case 0x8000: case 0x8001: case 0x8002: case 0x8003:
      programBank[0] = data.bit(0,4);
      break;
    case 0x9000: case 0x9001:
      mirror = data.bit(0,1);
      break;
    case 0x9002: case 0x9003:
      programMode = data.bit(1);
      break;
    case 0xa000: case 0xa001: case 0xa002: case 0xa003:
      programBank[1] = data.bit(0,4);
      break;
    case 0xb000: characterBank[0].bit(0,3) = data.bit(0,3); break;
    case 0xb001: characterBank[0].bit(4,7) = data.bit(0,3); break;
    case 0xb002: characterBank[1].bit(0,3) = data.bit(0,3); break;
    case 0xb003: characterBank[1].bit(4,7) = data.bit(0,3); break;
    case 0xc000: characterBank[2].bit(0,3) = data.bit(0,3); break;
    case 0xc001: characterBank[2].bit(4,7) = data.bit(0,3); break;
    case 0xc002: characterBank[3].bit(0,3) = data.bit(0,3); break;
    case 0xc003: characterBank[3].bit(4,7) = data.bit(0,3); break;
    case 0xd000: characterBank[4].bit(0,3) = data.bit(0,3); break;
    case 0xd001: characterBank[4].bit(4,7) = data.bit(0,3); break;
    case 0xd002: characterBank[5].bit(0,3) = data.bit(0,3); break;
    case 0xd003: characterBank[5].bit(4,7) = data.bit(0,3); break;
    case 0xe000: characterBank[6].bit(0,3) = data.bit(0,3); break;
    case 0xe001: characterBank[6].bit(4,7) = data.bit(0,3); break;
    case 0xe002: characterBank[7].bit(0,3) = data.bit(0,3); break;
    case 0xe003: characterBank[7].bit(4,7) = data.bit(0,3); break;
    case 0xf000: irqLatch.bit(0,3) = data.bit(0,3); break;
    case 0xf001: irqLatch.bit(4,7) = data.bit(0,3); break;
    case 0xf002:
      irqAcknowledge = data.bit(0);
      irqEnable = data.bit(1);
      irqMode = data.bit(2);
      if(irqEnable) {
        irqCounter = irqLatch;
        irqScalar = 341;
      }
      irqLine = 0;
      break;
    case 0xf003:
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
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(programMode);
    s(programBank);
    s(mirror);
    s(characterBank);
    s(irqLatch);
    s(irqMode);
    s(irqEnable);
    s(irqAcknowledge);
    s(irqCounter);
    s(irqScalar);
    s(irqLine);
  }

  n1  programMode;
  n5  programBank[2];
  n2  mirror;
  n8  characterBank[8];
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
