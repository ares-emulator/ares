struct AveNina06 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "AVE-NINA-03") return new AveNina06(Revision::N03);
    if(id == "AVE-NINA-06") return new AveNina06(Revision::N06);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    N03,
    N06,
  } revision;

  AveNina06(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(programBank << 15 | (n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if((address & 0xe100) == 0x4100) {
      programBank = data.bit(3);
      characterBank = data.bit(0,2);
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    return characterROM.read(characterBank << 13 | (n13)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto power() -> void override {
    programBank = 0;
    characterBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
  }

  n1 mirror;  //0 = horizontal, 1 = vertical
  n1 programBank;
  n4 characterBank;
};
