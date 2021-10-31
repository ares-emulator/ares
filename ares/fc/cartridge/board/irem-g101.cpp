struct IremG101 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "IREM-G101" ) return new IremG101(Revision::G101);
    if(id == "IREM-G101A") return new IremG101(Revision::G101A);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    G101,
    G101A,
  } revision;

  IremG101(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000 && !programRAM) return data;
    if(address < 0x8000) return programRAM.read((n13)address);

    n5 bank;
    switch(address >> 13 & 3) {
    case 0: bank = (programMode == 0 ? programBank[0] : (n5)0x1e); break;
    case 1: bank = programBank[1]; break;
    case 2: bank = (programMode == 1 ? programBank[0] : (n5)0x1e); break;
    case 3: bank = 0x1f; break;
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000 && !programRAM) return;
    if(address < 0x8000) return programRAM.write((n13)address, data);

    switch(address & 0xf000) {
    case 0x8000: programBank[0] = data.bit(0,4); break;
    case 0xa000: programBank[1] = data.bit(0,4); break;
    case 0xb000: characterBank[address & 7] = data; break;
    case 0x9000:
      if(revision == Revision::G101) {
        mirror = data.bit(0);
        programMode = data.bit(1);
      }
      break;
	  }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::G101A) return (n10)address;
    return address >> mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) {
      n8 bank = characterBank[address >> 10 & 7];
      return characterROM.read(bank << 10 | (n10)address);
    }
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(programBank);
    s(characterBank);
    s(programMode);
    s(mirror);
  }

  n5 programBank[2];
  n8 characterBank[8];
  n1 programMode;
  n1 mirror;
};
