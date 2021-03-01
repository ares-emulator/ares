struct KonamiVRC2 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "KONAMI-VRC-2") return new KonamiVRC2;
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

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;

    if(address < 0x8000) {
      if(!programRAM && (address & 0xf000) == 0x6000) return data | latch;
      if(!programRAM) return data;
      return programRAM.read((n13)address);
    }

    n5 bank, banks = programROM.size() >> 13;
    switch(address & 0xe000) {
    case 0x8000: bank = programBank[0]; break;
    case 0xa000: bank = programBank[1]; break;
    case 0xc000: bank = banks - 2; break;
    case 0xe000: bank = banks - 1; break;
    }
    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;

    if(address < 0x8000) {
      if(!programRAM && (address & 0xf000) == 0x6000) latch = data.bit(0);
      if(!programRAM) return;
      return programRAM.write((n13)address, data);
    }

    bool a0 = address & pinA0;
    bool a1 = address & pinA1;
    address &= 0xf000;
    address |= a0 << 0 | a1 << 1;

    switch(address) {
    case 0x8000: case 0x8001: case 0x8002: case 0x8003:
      programBank[0] = data.bit(0,4);
      break;
    case 0x9000: case 0x9001: case 0x9002: case 0x9003:
      mirror = data.bit(0,1);
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
    n8 bank = characterBank[address >> 10];
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
    s(programBank);
    s(characterBank);
    s(mirror);
    s(latch);
  }

  n5 programBank[2];
  n8 characterBank[8];
  n2 mirror;
  n1 latch;

//unserialized:
  n8 pinA0;
  n8 pinA1;
};
