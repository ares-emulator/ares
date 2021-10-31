struct IremIF12 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "IREM-IF-12"  ) return new IremIF12(Revision::IF12);
    if(id == "JALECO-JF-16") return new IremIF12(Revision::JF16);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    IF12,
    JF16,
  } revision;

  IremIF12(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n3 bank;
    switch(address >> 14 & 1) {
    case 0: bank = programBank; break;
    case 1: bank = 7; break;
    }
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank   = data.bit(0,2);
    mirror        = data.bit(3);
    characterBank = data.bit(4,7);
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::JF16) {
      return mirror << 10 | (n10)address;
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
  }

  n1 mirror;
  n3 programBank;
  n4 characterBank;
};
