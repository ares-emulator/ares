struct Namco340 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "NAMCO-175") return new Namco340(Revision::N175);
    if(id == "NAMCO-340") return new Namco340(Revision::N340);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    N175,
    N340,
  } revision;

  Namco340(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    staticMirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) {
      if(!ramEnable || !programRAM) return data;
      return programRAM.read((n13)address);
    }

    n6 bank;
    switch(address >> 13 & 3) {
    case 0: bank = programBank[0]; break;
    case 1: bank = programBank[1]; break;
    case 2: bank = programBank[2]; break;
    case 3: bank = 0x3f; break;
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) {
      if(!ramEnable || !programRAM) return;
      return programRAM.write((n13)address, data);
    }

    switch(address & 0xf800) {
    case 0x8000: characterBank[0] = data; break;
    case 0x8800: characterBank[1] = data; break;
    case 0x9000: characterBank[2] = data; break;
    case 0x9800: characterBank[3] = data; break;
    case 0xa000: characterBank[4] = data; break;
    case 0xa800: characterBank[5] = data; break;
    case 0xb000: characterBank[6] = data; break;
    case 0xb800: characterBank[7] = data; break;
    case 0xc000: ramEnable = data.bit(0); break;
    case 0xe000: programBank[0] = data.bit(0,5); mirror = data.bit(6,7); break;
    case 0xe800: programBank[1] = data.bit(0,5); break;
    case 0xf000: programBank[2] = data.bit(0,5); break;
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::N175) {
      return address >> !staticMirror & 0x0400 | (n10)address;
    }

    switch(mirror) {
    case 0: return 0x0000 | (n10)address;
    case 1: return address >> 0 & 0x0400 | (n10)address;
    case 2: return 0x0400 | (n10)address;
    case 3: return address >> 1 & 0x0400 | (n10)address;
    }
    unreachable;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    n8 bank = characterBank[address >> 10 & 7];
    return characterROM.read(bank << 10 | (n10)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(mirror);
    s(staticMirror);
    s(ramEnable);
    s(programBank);
    s(characterBank);
  }

  n2 mirror;
  n1 staticMirror;
  n1 ramEnable;
  n6 programBank[3];
  n8 characterBank[8];
};
