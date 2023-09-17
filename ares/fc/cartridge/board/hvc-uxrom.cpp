struct HVC_UxROM : Interface {
  static auto create(string id) -> Interface* {
    if(id == "HVC-UNROM" ) return new HVC_UxROM(Revision::UNROM);
    if(id == "HVC-UNROMA") return new HVC_UxROM(Revision::UNROMA);
    if(id == "HVC-UN1ROM") return new HVC_UxROM(Revision::UN1ROM);
    if(id == "HVC-UOROM" ) return new HVC_UxROM(Revision::UOROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
    UNROM,
    UNROMA,
    UN1ROM,
    UOROM,
  } revision;

  HVC_UxROM(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  inline auto bankPRG(n32 address) -> n8 {
    switch(address >> 14 & 1) {
    case 0: return (revision == Revision::UNROMA ? (n8)0x00 : programBank); break;
    case 1: default: return (revision == Revision::UNROMA ? programBank : (n8)0xff); break;
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(bankPRG(address) << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank = data;
    if(revision == Revision::UN1ROM) programBank >>= 2;
  }

  auto debugAddress(n32 address) -> n32 override {
    if(address < 0x8000) return address;
    return (bankPRG(address) << 16 | (n16)address) & ((bit::round(programROM.size()) << 2) - 1);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(mirror);
    s(programBank);
  }

  n1 mirror;  //0 = horizontal, 1 = vertical
  n8 programBank;
};
