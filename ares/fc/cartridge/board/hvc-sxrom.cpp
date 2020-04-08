struct HVC_SxROM : Interface {  //MMC1
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;

  enum class Revision : uint {
    SAROM,
    SBROM,
    SCROM,
    SC1ROM,
    SEROM,
    SFROM,
    SFEXPROM,
    SGROM,
    SHROM,
    SH1ROM,
    SIROM,
    SJROM,
    SKROM,
    SLROM,
    SL1ROM,
    SL2ROM,
    SL3ROM,
    SLRROM,
    SMROM,
    SNROM,
    SOROM,
    SUROM,
    SXROM,
  } revision;

  enum class ChipRevision : uint {
    MMC1,
    MMC1A,
    MMC1B1,
    MMC1B2,
    MMC1B3,
    MMC1C,
  } chipRevision;

  HVC_SxROM(Markup::Node document, Revision revision) : Interface(document), revision(revision) {}

  auto load(Markup::Node document) -> void override {
    chipRevision = ChipRevision::MMC1B2;

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

  auto main() -> void {
    if(writeDelay) writeDelay--;
    tick();
  }

  auto addressProgramROM(uint address) -> uint {
    bool region = address & 0x4000;
    uint5 bank = programBank & ~1 | region;
    if(programSize == 1) {
      bank = (region == 0 ? 0x0 : 0xf);
      if(region != programMode) bank = programBank;
    }
    if(revision == Revision::SXROM) {
      bank.bit(4) = characterBank[0].bit(4);
    }
    return bank << 14 | (uint14)address;
  }

  auto addressProgramRAM(uint address) -> uint {
    uint bank = 0;
    if(revision == Revision::SOROM) bank = characterBank[0].bit(3);
    if(revision == Revision::SUROM) bank = characterBank[0].bit(2,3);
    if(revision == Revision::SXROM) bank = characterBank[0].bit(2,3);
    return bank << 13 | (uint13)address;
  }

  auto addressCHR(uint address) -> uint {
    bool region = address & 0x1000;
    uint5 bank = characterBank[region];
    if(characterMode == 0) bank = characterBank[0] & ~1 | region;
    return bank << 12 | (uint12)address;
  }

  auto addressCIRAM(uint address) -> uint {
    switch(mirrorMode) {
    case 0: return 0x0000 | address & 0x03ff;
    case 1: return 0x0400 | address & 0x03ff;
    case 2: return address >> 0 & 0x0400 | address & 0x03ff;
    case 3: return address >> 1 & 0x0400 | address & 0x03ff;
    }
    unreachable;
  }

  auto readPRG(uint address) -> uint8 {
    if((address & 0xe000) == 0x6000) {
      if(revision == Revision::SNROM) {
        if(characterBank[0].bit(4)) return cpu.mdr();
      }
      if(ramDisable) return 0x00;
      return programRAM.read(addressProgramRAM(address));
    }

    if(address & 0x8000) {
      return programROM.read(addressProgramROM(address));
    }

    return cpu.mdr();
  }

  auto writePRG(uint address, uint8 data) -> void {
    if((address & 0xe000) == 0x6000) {
      if(revision == Revision::SNROM) {
        if(characterBank[0].bit(4)) return;
      }
      if(ramDisable) return;
      return programRAM.write(addressProgramRAM(address), data);
    }

    if(address & 0x8000) return writeIO(address, data);
  }

  auto writeIO(uint address, uint8 data) -> void {
    if(writeDelay) return;
    writeDelay = 2;

    if(data.bit(7)) {
      shiftCount = 0;
      programSize = 1;
      programMode = 1;
    } else {
      shiftValue = data.bit(0) << 4 | shiftValue >> 1;
      if(++shiftCount == 5) {
        shiftCount = 0;
        switch(address >> 13 & 3) {
        case 0:
          mirrorMode = shiftValue.bit(0,1);
          programMode = shiftValue.bit(2);
          programSize = shiftValue.bit(3);
          characterMode = shiftValue.bit(4);
          break;
        case 1:
          characterBank[0] = shiftValue.bit(0,4);
          break;
        case 2:
          characterBank[1] = shiftValue.bit(0,4);
          break;
        case 3:
          programBank = shiftValue.bit(0,3);
          ramDisable = shiftValue.bit(4);
          break;
        }
      }
    }
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
    writeDelay = 0;
    shiftCount = 0;
    shiftValue = 0;
    mirrorMode = 0;
    programMode = 1;
    programSize = 1;
    characterMode = 0;
    characterBank[0] = 0;
    characterBank[1] = 1;
    programBank = 0;
    ramDisable = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    characterRAM.serialize(s);
    s.integer(writeDelay);
    s.integer(shiftCount);
    s.integer(shiftValue);
    s.integer(mirrorMode);
    s.integer(programMode);
    s.integer(programSize);
    s.integer(characterMode);
    s.array(characterBank);
    s.integer(programBank);
    s.integer(ramDisable);
  }

  uint8 writeDelay;
  uint8 shiftCount;
  uint5 shiftValue;
  uint2 mirrorMode;
  uint1 programMode;
  uint1 programSize;
  uint1 characterMode;
  uint5 characterBank[2];
  uint4 programBank;
  uint1 ramDisable;
};
