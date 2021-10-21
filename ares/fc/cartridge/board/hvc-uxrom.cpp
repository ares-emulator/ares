struct HVC_UxROM : Interface {
  static auto create(string id) -> Interface* {
    if(id == "HVC-UNROM") return new HVC_UxROM(Revision::UNROM);
    if(id == "HVC-UOROM") return new HVC_UxROM(Revision::UOROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
    UNROM,
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

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    n8 fixedBank = programROM.size() == 512_KiB ? 0x1f : 0xf;
    n8 bank = (address < 0xc000 ? programBank : fixedBank);
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank = data;
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
