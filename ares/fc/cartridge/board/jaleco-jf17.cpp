//todo: uPD7756 ADPCM unsupported

struct JalecoJF17 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "JALECO-JF-17") return new JalecoJF17(Revision::JF17);
    if(id == "JALECO-JF-19") return new JalecoJF17(Revision::JF19);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    JF17,
    JF19,
  } revision;

  JalecoJF17(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n4 bank;
    switch(address >> 14 & 1) {
    case 0: bank = (revision == Revision::JF17 ? programBank : (n4)0x0); break;
    case 1: bank = (revision == Revision::JF19 ? programBank : (n4)0xf); break;
    }
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    if(!programSelect && data.bit(7)) programBank = data.bit(0,3);
    if(!characterSelect && data.bit(6)) characterBank = data.bit(0,3);
    programSelect = data.bit(7);
    characterSelect = data.bit(6);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    address = characterBank << 13 | (n13)address;
    return characterROM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto serialize(serializer& s) -> void override {
    s(mirror);
    s(programSelect);
    s(programBank);
    s(characterSelect);
    s(characterBank);
  }

  n1 mirror;
  n1 programSelect;
  n4 programBank;
  n1 characterSelect;
  n4 characterBank;
};
