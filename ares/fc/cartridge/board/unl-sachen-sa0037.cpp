struct SachenSA0037 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "TENGEN-800008") return new SachenSA0037(Revision::TENGEN_800008);
    if(id == "UNL-SA-0037"  ) return new SachenSA0037(Revision::SACHEN_SA0037);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    SACHEN_SA0037,
    TENGEN_800008,
  } revision;

  SachenSA0037(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(programBank << 15 | (n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    characterBank = data.bit(0,2);
    programBank = data.bit(3);
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
  n1 programBank;
  n3 characterBank;
};
