struct MltAction52 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "MLT-ACTION52") return new MltAction52();
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

 auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n2 chipOffset;
    switch(programChip) {
    case 0: chipOffset = 0; break;
    case 1: chipOffset = 1; break;
    case 2: return data;
    case 3: chipOffset = 2; break;
    }

    bool region = address.bit(14);
    n5 bank = programMode ? programBank : (n5)(programBank & ~1 | region);
    address = chipOffset << 19 | bank << 14 | (n14)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;

    characterBank = address.bit(0,3) << 2 | data.bit(0,1);
    programMode = address.bit(5);
    programBank = address.bit(6,10);
    programChip = address.bit(11,12);
    mirror = address.bit(13);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    address = characterBank << 13 | (n13)address;
    if(characterROM) return characterROM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(characterBank);
    s(programMode);
    s(programBank);
    s(programChip);
    s(mirror);
  }

  n6 characterBank;
  n1 programMode;
  n5 programBank;
  n2 programChip;
  n1 mirror;
};
