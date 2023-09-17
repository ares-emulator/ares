struct Action53 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "ACTION53") return new Action53();
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    
    n8 programBankMask = (1 << programBankWidth) - 1;
    if (programBankMode < 2) {
      n8 outerBank = programOuterBank & ~programBankMask;
      n8 innerBank = programBank & programBankMask;
      return programROM.read((outerBank | innerBank) << 15 | (n15)address);
    }

    n9 outerBank = (programOuterBank & ~programBankMask) << 1;
    n9 innerBank = programBank & ((2 << programBankWidth) - 1);
    n9 offset = (address >> 14) & 1;
    if(offset == (programBankMode & 1))
      return programROM.read((programOuterBank << 15) | (n15)address);
    return programROM.read((outerBank | innerBank) << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    // Register select
    if((address & 0xF000) == 0x5000) {
      currentRegister = data & 0x81;
      return;
    }
    
    // Register write
    if(address < 0x8000) return;
    switch(currentRegister) {
    case 0x00: { /* CHR bank */
      characterBank = data.bit(0, 1);
      if(!mirror) nametableBank = data.bit(4);
    } break;
    case 0x01: { /* PRG bank */
      programBank = data.bit(0, 3);
      if(!mirror) nametableBank = data.bit(4);
    } break;
    case 0x80: { /* Mode */
      nametableBank = data.bit(0);
      mirror = data.bit(1);
      programBankMode = data.bit(2, 3);
      programBankWidth = data.bit(4, 5);
    } break;
    case 0x81: { /* PRG outer bank */
      programOuterBank = data;
    } break;
    }
  }

  inline auto addressCIRAM(n32 address) -> n11 {
    if(mirror) {
      return ((address >> nametableBank) & 0x0400) | (n10)address;
    }
    return (nametableBank << 10) | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      return ppu.readCIRAM(addressCIRAM(address));
    }
    if(characterROM) return characterROM.read(characterBank << 13 | (n13)address);
    if(characterRAM) return characterRAM.read(characterBank << 13 | (n13)address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      return ppu.writeCIRAM(addressCIRAM(address), data);
    }
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
    currentRegister = 0;
    characterBank = 0;
    programBank = 0;
    programOuterBank = 0xFF;
    nametableBank = 0;
    mirror = 0;
    programBankMode = 0;
    programBankWidth = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(currentRegister);
    s(characterBank);
    s(programBank);
    s(programOuterBank);
    s(nametableBank);
    s(mirror);
    s(programBankMode);
    s(programBankWidth);
  }

  n8 currentRegister;
  n2 characterBank;
  n4 programBank;
  n8 programOuterBank;
  n1 nametableBank;
  n1 mirror;
  n2 programBankMode;
  n2 programBankWidth;
};
