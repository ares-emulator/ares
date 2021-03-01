struct HVC_SxROM : Interface {  //MMC1
  static auto create(string id) -> Interface* {
    if(id == "HVC-SAROM"   ) return new HVC_SxROM(Revision::SAROM);
    if(id == "HVC-SBROM"   ) return new HVC_SxROM(Revision::SBROM);
    if(id == "HVC-SCROM"   ) return new HVC_SxROM(Revision::SCROM);
    if(id == "HVC-SC1ROM"  ) return new HVC_SxROM(Revision::SC1ROM);
    if(id == "HVC-SEROM"   ) return new HVC_SxROM(Revision::SEROM);
    if(id == "HVC-SFROM"   ) return new HVC_SxROM(Revision::SFROM);
    if(id == "HVC-SFEXPROM") return new HVC_SxROM(Revision::SFEXPROM);
    if(id == "HVC-SGROM"   ) return new HVC_SxROM(Revision::SGROM);
    if(id == "HVC-SHROM"   ) return new HVC_SxROM(Revision::SHROM);
    if(id == "HVC-SH1ROM"  ) return new HVC_SxROM(Revision::SH1ROM);
    if(id == "HVC-SIROM"   ) return new HVC_SxROM(Revision::SIROM);
    if(id == "HVC-SJROM"   ) return new HVC_SxROM(Revision::SJROM);
    if(id == "HVC-SKROM"   ) return new HVC_SxROM(Revision::SKROM);
    if(id == "HVC-SLROM"   ) return new HVC_SxROM(Revision::SKROM);
    if(id == "HVC-SL1ROM"  ) return new HVC_SxROM(Revision::SL1ROM);
    if(id == "HVC-SL2ROM"  ) return new HVC_SxROM(Revision::SL2ROM);
    if(id == "HVC-SL3ROM"  ) return new HVC_SxROM(Revision::SL3ROM);
    if(id == "HVC-SLRROM"  ) return new HVC_SxROM(Revision::SLRROM);
    if(id == "HVC-SMROM"   ) return new HVC_SxROM(Revision::SMROM);
    if(id == "HVC-SNROM"   ) return new HVC_SxROM(Revision::SNROM);
    if(id == "HVC-SOROM"   ) return new HVC_SxROM(Revision::SOROM);
    if(id == "HVC-SUROM"   ) return new HVC_SxROM(Revision::SUROM);
    if(id == "HVC-SXROM"   ) return new HVC_SxROM(Revision::SXROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
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

  enum class ChipRevision : u32 {
    MMC1,
    MMC1A,
    MMC1B1,
    MMC1B2,
    MMC1B3,
    MMC1C,
  } chipRevision;

  HVC_SxROM(Revision revision) : revision(revision) {}

  auto load() -> void override {
    chipRevision = ChipRevision::MMC1B2;

    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
  }

  auto main() -> void override {
    if(writeDelay) writeDelay--;
    tick();
  }

  auto addressProgramROM(n32 address) -> n32 {
    bool region = address & 0x4000;
    n5 bank = programBank & ~1 | region;
    if(programSize == 1) {
      bank = (region == 0 ? 0x0 : 0xf);
      if(region != programMode) bank = programBank;
    }
    if(revision == Revision::SXROM) {
      bank.bit(4) = characterBank[0].bit(4);
    }
    return bank << 14 | (n14)address;
  }

  auto addressProgramRAM(n32 address) -> n32 {
    n32 bank = 0;
    if(revision == Revision::SOROM) bank = characterBank[0].bit(3);
    if(revision == Revision::SUROM) bank = characterBank[0].bit(2,3);
    if(revision == Revision::SXROM) bank = characterBank[0].bit(2,3);
    return bank << 13 | (n13)address;
  }

  auto addressCHR(n32 address) -> n32 {
    bool region = address & 0x1000;
    n5 bank = characterBank[region];
    if(characterMode == 0) bank = characterBank[0] & ~1 | region;
    return bank << 12 | (n12)address;
  }

  auto addressCIRAM(n32 address) -> n32 {
    switch(mirrorMode) {
    case 0: return 0x0000 | address & 0x03ff;
    case 1: return 0x0400 | address & 0x03ff;
    case 2: return address >> 0 & 0x0400 | address & 0x03ff;
    case 3: return address >> 1 & 0x0400 | address & 0x03ff;
    }
    unreachable;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if((address & 0xe000) == 0x6000) {
      if(revision == Revision::SNROM) {
        if(characterBank[0].bit(4)) return data;
      }
      if(ramDisable) return 0x00;
      return programRAM.read(addressProgramRAM(address));
    }

    if(address & 0x8000) {
      return programROM.read(addressProgramROM(address));
    }

    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if((address & 0xe000) == 0x6000) {
      if(revision == Revision::SNROM) {
        if(characterBank[0].bit(4)) return;
      }
      if(ramDisable) return;
      return programRAM.write(addressProgramRAM(address), data);
    }

    if(address & 0x8000) return writeIO(address, data);
  }

  auto writeIO(n32 address, n8 data) -> void {
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
    programMode = 1;
    programSize = 1;
    characterBank[1] = 1;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(writeDelay);
    s(shiftCount);
    s(shiftValue);
    s(mirrorMode);
    s(programMode);
    s(programSize);
    s(characterMode);
    s(characterBank);
    s(programBank);
    s(ramDisable);
  }

  n8 writeDelay;
  n8 shiftCount;
  n5 shiftValue;
  n2 mirrorMode;
  n1 programMode;
  n1 programSize;
  n1 characterMode;
  n5 characterBank[2];
  n4 programBank;
  n1 ramDisable;
};
