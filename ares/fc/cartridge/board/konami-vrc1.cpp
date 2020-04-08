struct KonamiVRC1 : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x8000) return cpu.mdr();

    uint4 bank;
    if((address & 0xe000) == 0x8000) bank = programBank[0];
    if((address & 0xe000) == 0xa000) bank = programBank[1];
    if((address & 0xe000) == 0xc000) bank = programBank[2];
    if((address & 0xe000) == 0xe000) bank = 0xf;
    return programROM.read(bank << 13 | (uint13)address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x8000) return;

    switch(address & 0xf000) {
    case 0x8000:
      programBank[0] = data.bit(0,3);
      break;
    case 0x9000:
      mirror = data.bit(0);
      characterBank[0].bit(4) = data.bit(1);
      characterBank[1].bit(4) = data.bit(2);
      break;
    case 0xa000:
      programBank[1] = data.bit(0,3);
      break;
    case 0xc000:
      programBank[2] = data.bit(0,3);
      break;
    case 0xe000:
      characterBank[0].bit(0,3) = data.bit(0,3);
      break;
    case 0xf000:
      characterBank[1].bit(0,3) = data.bit(0,3);
      break;
    }
  }

  auto addressCIRAM(uint address) const -> uint {
    return address >> mirror & 0x0400 | address & 0x03ff;
  }

  auto addressCHR(uint address) const -> uint {
    uint1 bank = bool(address & 0x1000);
    return characterBank[bank] << 12 | (uint12)address;
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
  }

  auto serialize(serializer& s) -> void {
    characterRAM.serialize(s);
    s.array(programBank);
    s.array(characterBank);
    s.integer(mirror);
  }

  uint4 programBank[3];
  uint5 characterBank[2];
  uint1 mirror;
};
