//todo: uPD7756 ADPCM unsupported

struct JalecoJF11 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "JALECO-JF-11") return new JalecoJF11(Revision::JF11);
    if(id == "JALECO-JF-13") return new JalecoJF11(Revision::JF13);
    if(id == "JALECO-JF-14") return new JalecoJF11(Revision::JF14);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    JF11,
    JF13,
    JF14,
  } revision;

  JalecoJF11(Revision revision) : revision(revision) {}

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
    if(address < 0x6000 || address > 0x7fff) return;
    if(revision == Revision::JF13 && address > 0x6fff) return;

    programBank = data.bit(4,5);
    if(revision == Revision::JF13) {
      characterBank = data.bit(6) << 2 | data.bit(0,1);
    } else {
      characterBank = data.bit(0,3);
    }
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
    s(programBank);
    s(characterBank);
  }

  n1 mirror;  //0 = horizontal, 1 = vertical
  n2 programBank;
  n4 characterBank;
};
