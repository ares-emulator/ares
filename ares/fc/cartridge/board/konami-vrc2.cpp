struct KonamiVRC2 : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
    pinA0 = 1 << board["chip(type=VRC2)/pinout/a0"].natural();
    pinA1 = 1 << board["chip(type=VRC2)/pinout/a1"].natural();
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x6000) return cpu.mdr();

    if(address < 0x8000) {
      if(!programRAM && (address & 0xf000) == 0x6000) return cpu.mdr() | latch;
      if(!programRAM) return cpu.mdr();
      return programRAM.read((uint13)address);
    }

    uint5 bank, banks = programROM.size() >> 13;
    switch(address & 0xe000) {
    case 0x8000: bank = programBank[0]; break;
    case 0xa000: bank = programBank[1]; break;
    case 0xc000: bank = banks - 2; break;
    case 0xe000: bank = banks - 1; break;
    }
    address = bank << 13 | (uint13)address;
    return programROM.read(address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x6000) return;

    if(address < 0x8000) {
      if(!programRAM && (address & 0xf000) == 0x6000) latch = data.bit(0);
      if(!programRAM) return;
      return programRAM.write((uint13)address, data);
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
    uint8 bank = characterBank[address >> 10];
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
    for(auto& bank : programBank) bank = 0;
    for(auto& bank : characterBank) bank = 0;
    mirror = 0;
    latch = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    characterRAM.serialize(s);
    s.integer(pinA0);
    s.integer(pinA1);
    s.array(programBank);
    s.array(characterBank);
    s.integer(mirror);
    s.integer(latch);
  }

  uint8 pinA0;
  uint8 pinA1;
  uint5 programBank[2];
  uint8 characterBank[8];
  uint2 mirror;
  uint1 latch;
};
