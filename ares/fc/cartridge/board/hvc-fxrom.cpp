struct HVC_FxROM : Interface {  //MMC4
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;

  enum class Revision : uint {
    FJROM,
    FKROM,
  } revision;

  HVC_FxROM(Markup::Node document, Revision revision) : Interface(document), revision(revision) {}

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x6000) return cpu.mdr();
    if(address < 0x8000) return programRAM.read(address);
    uint4 bank = address < 0xc000 ? programBank : (uint4)0xf;
    return programROM.read(bank << 14 | (uint14)address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write(address, data);

    switch(address & 0xf000) {
    case 0xa000: programBank = data.bit(0,3); break;
    case 0xb000: characterBank[0][0] = data.bit(0,4); break;
    case 0xc000: characterBank[0][1] = data.bit(0,4); break;
    case 0xd000: characterBank[1][0] = data.bit(0,4); break;
    case 0xe000: characterBank[1][1] = data.bit(0,4); break;
    case 0xf000: mirror = data.bit(0); break;
    }
  }

  auto addressCIRAM(uint address) const -> uint {
    return address >> mirror & 0x0400 | address & 0x03ff;
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    uint1 region = bool(address & 0x1000);
    uint5 bank = characterBank[region][latch[region]];
    if((address & 0x0ff8) == 0x0fd8) latch[region] = 0;
    if((address & 0x0ff8) == 0x0fe8) latch[region] = 1;
    address = bank << 12 | (uint12)address;
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return 0x00;
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    uint1 region = bool(address & 0x1000);
    uint5 bank = characterBank[region][latch[region]];
    if((address & 0x0ff8) == 0x0fd8) latch[region] = 0;
    if((address & 0x0ff8) == 0x0fe8) latch[region] = 1;
    address = bank << 12 | (uint12)address;
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void {
    programBank = 0;
    characterBank[0][0] = 0;
    characterBank[0][1] = 0;
    characterBank[1][0] = 0;
    characterBank[1][1] = 0;
    mirror = 0;
    latch[0] = 0;
    latch[1] = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    characterRAM.serialize(s);
    s.integer(programBank);
    s.integer(characterBank[0][0]);
    s.integer(characterBank[0][1]);
    s.integer(characterBank[1][0]);
    s.integer(characterBank[1][1]);
    s.integer(mirror);
    s.array(latch);
  }

  uint4 programBank;
  uint5 characterBank[2][2];
  uint1 mirror;
  uint1 latch[2];
};
