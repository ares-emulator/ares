struct HVC_PxROM : Interface {  //MMC2
  static auto create(string id) -> Interface* {
    if(id == "HVC-PEEOROM") return new HVC_PxROM(Revision::PEEOROM);
    if(id == "HVC-PNROM"  ) return new HVC_PxROM(Revision::PNROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum Revision : u32 {
    PEEOROM,
    PNROM,
  } revision;

  HVC_PxROM(Revision revision) : revision(revision) {}

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
    if(address < 0x8000) return programRAM.read((n13)address);

    n4 bank = 0;
    switch(address & 0xe000) {
    case 0x8000: bank = programBank; break;
    case 0xa000: bank = 0xd; break;
    case 0xc000: bank = 0xe; break;
    case 0xe000: bank = 0xf; break;
    }
    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write((n13)address, data);

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
    return address >> mirror & 0x0400 | (n10)address;
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
