struct Namco118 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "NAMCO-118" ) return new Namco118(Revision::N118);
    if(id == "NAMCO-3425") return new Namco118(Revision::N3425);
    if(id == "NAMCO-3433") return new Namco118(Revision::N3433);
    if(id == "NAMCO-3446") return new Namco118(Revision::N3446);
    if(id == "NAMCO-3453") return new Namco118(Revision::N3453);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterCIRAM;

  enum class Revision : u32 {
    N118,
    N3425,
    N3433,
    N3446,
    N3453,
  } revision;

  Namco118(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterCIRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(characterCIRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n4 bank;
    switch(address >> 13 & 3) {
    case 0: bank = programBank[0]; break;
    case 1: bank = programBank[1]; break;
    case 2: bank = 0x0e; break;
    case 3: bank = 0x0f; break;
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x8000) nametableBank = data.bit(6);

    switch(address & 0xe001) {
    case 0x8000:
      bankSelect = data.bit(0,2);
      break;
    case 0x8001:
      switch(bankSelect) {
      case 0: characterBank[0] = data >> 1; break;
      case 1: characterBank[1] = data >> 1; break;
      case 2: characterBank[2] = data; break;
      case 3: characterBank[3] = data; break;
      case 4: characterBank[4] = data; break;
      case 5: characterBank[5] = data; break;
      case 6: programBank[0] = data; break;
      case 7: programBank[1] = data; break;
      }
      break;
    }
  }

  auto addressCHR(n32 address) const -> n32 {
    n32 chrAddress = 0;
    if(revision == Revision::N3446) {
      switch(address) {
      case 0x0000 ... 0x07ff: chrAddress = characterBank[2] << 11 | (n11)address; break;
      case 0x0800 ... 0x0fff: chrAddress = characterBank[3] << 11 | (n11)address; break;
      case 0x1000 ... 0x17ff: chrAddress = characterBank[4] << 11 | (n11)address; break;
      case 0x1800 ... 0xffff: chrAddress = characterBank[5] << 11 | (n11)address; break;
      }
    } else {
      switch(address) {
      case 0x0000 ... 0x07ff: chrAddress = characterBank[0] << 11 | (n11)address; break;
      case 0x0800 ... 0x0fff: chrAddress = characterBank[1] << 11 | (n11)address; break;
      case 0x1000 ... 0x13ff: chrAddress = characterBank[2] << 10 | (n10)address; break;
      case 0x1400 ... 0x17ff: chrAddress = characterBank[3] << 10 | (n10)address; break;
      case 0x1800 ... 0x1bff: chrAddress = characterBank[4] << 10 | (n10)address; break;
      case 0x1c00 ... 0x1fff: chrAddress = characterBank[5] << 10 | (n10)address; break;
      }
      chrAddress |= (address & 0x1000) << 4;
    }
    return chrAddress;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::N3425) {
      n6 bank = characterBank[address >> 11 & 1];
      return bank.bit(4) << 10 | (n10)address;
    }
    if(revision == Revision::N3453) {
      return nametableBank << 10 | (n10)address;
    }
    return address >> !mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      if(!characterCIRAM) return ppu.readCIRAM(addressCIRAM(address));
      if(address & 0x0800) return characterCIRAM.read((n11)address);
      return ppu.readCIRAM((n11)address);
    }
    return characterROM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      if(!characterCIRAM) return ppu.writeCIRAM(addressCIRAM(address), data);
      if(address & 0x0800) return characterCIRAM.write((n11)address, data);
      return ppu.writeCIRAM((n11)address, data);
    }
  }

  auto serialize(serializer& s) -> void override {
    s(characterCIRAM);
    s(mirror);
    s(bankSelect);
    s(programBank);
    s(characterBank);
    s(nametableBank);
  }

  n1 mirror;
  n3 bankSelect;
  n4 programBank[2];
  n6 characterBank[6];
  n1 nametableBank;
};
