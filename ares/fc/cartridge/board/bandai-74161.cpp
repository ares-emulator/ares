struct Bandai74161 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "BANDAI-74161" ) return new Bandai74161(Revision::B74161);
    if(id == "BANDAI-74161A") return new Bandai74161(Revision::B74161A);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    B74161,
    B74161A,
  } revision;

  Bandai74161(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    n3 bank = (address < 0xc000 ? programBank : (n3)0x7);
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    characterBank = data.bit(0,3);
    programBank   = data.bit(4,6);
    nametableBank = data.bit(7);
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::B74161A) {
      return nametableBank << 10 | (n10)address;
    }
    return address >> !mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterROM.read(characterBank << 13 | (n13)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(mirror);
    s(programBank);
    s(characterBank);
    s(nametableBank);
  }

  n1 mirror;
  n3 programBank;
  n4 characterBank;
  n1 nametableBank;
};
