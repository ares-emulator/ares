struct HVC_FxROM : Interface {  //MMC4
  static auto create(string id) -> Interface* {
    if(id == "HVC-FJROM") return new HVC_FxROM(Revision::FJROM);
    if(id == "HVC-FKROM") return new HVC_FxROM(Revision::FKROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
    FJROM,
    FKROM,
  } revision;

  HVC_FxROM(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) return programRAM.read(address);
    n4 bank = address < 0xc000 ? programBank : (n4)0xf;
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write(address, data);

    switch(address & 0xf000) {
    case 0xa000: programBank = data.bit(0,3); break;
    case 0xb000: characterBank[0][0] = data.bit(0,4); break;
    case 0xc000: characterBank[0][1] = data.bit(0,4); break;
    case 0xd000: characterBank[1][0] = data.bit(0,4); break;
    case 0xe000: characterBank[1][1] = data.bit(0,4); break;
    case 0xf000: mirror = data.bit(0); break;
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    return address >> mirror & 0x0400 | address & 0x03ff;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    n1 region = bool(address & 0x1000);
    n5 bank = characterBank[region][latch[region]];
    if((address & 0x0ff8) == 0x0fd8) latch[region] = 0;
    if((address & 0x0ff8) == 0x0fe8) latch[region] = 1;
    address = bank << 12 | (n12)address;
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    n1 region = bool(address & 0x1000);
    n5 bank = characterBank[region][latch[region]];
    if((address & 0x0ff8) == 0x0fd8) latch[region] = 0;
    if((address & 0x0ff8) == 0x0fe8) latch[region] = 1;
    address = bank << 12 | (n12)address;
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(programBank);
    s(characterBank[0][0]);
    s(characterBank[0][1]);
    s(characterBank[1][0]);
    s(characterBank[1][1]);
    s(mirror);
    s(latch);
  }

  n4 programBank;
  n5 characterBank[2][2];
  n1 mirror;
  n1 latch[2];
};
