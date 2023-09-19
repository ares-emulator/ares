struct UNROM512 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "UNROM-512") return new UNROM512();
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");

    auto pakMirror = pak->attribute("mirror");
    mirror = pakMirror == "horizontal" ? 0 : (pakMirror == "vertical" ? 1 : (pakMirror == "external" ? 3 : 2));
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  inline auto bankPRG(n32 address) -> n5 {
    switch(address >> 14 & 1) {
    case 0: return programBank; break;
    case 1: default: return (n5)0x1f; break;
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(bankPRG(address) << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank = data.bit(0, 4);
    characterBank = data.bit(5, 6);
    nametableBank = data.bit(7);
  }

  auto debugAddress(n32 address) -> n32 override {
    if(address < 0x8000) return address;
    return (bankPRG(address) << 16 | (n16)address) & ((bit::round(programROM.size()) << 2) - 1);
  }

  inline auto addressCIRAM(n32 address) -> n11 {
    if(mirror & 2) {
      return (nametableBank << 10) | (n10)address;
    }
    return address >> !mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      if(mirror == 3) {
        if(characterROM) return characterROM.read(0x6000 | (n13)address);
        if(characterRAM) return characterRAM.read(0x6000 | (n13)address);
      }
      return ppu.readCIRAM(addressCIRAM(address));
    }
    if(characterROM) return characterROM.read(characterBank << 13 | (n13)address);
    if(characterRAM) return characterRAM.read(characterBank << 13 | (n13)address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      if(mirror == 3) {
        if(characterROM) return characterROM.write(0x6000 | (n13)address, data);
        if(characterRAM) return characterRAM.write(0x6000 | (n13)address, data);
      }
      return ppu.writeCIRAM(addressCIRAM(address), data);
    }
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(mirror);
    s(programBank);
    s(characterBank);
    s(nametableBank);
  }

  n2 mirror;  // 0 = horizontal, 1 = vertical, 2 = 1-screen banked, 3 = 4-screen
  n5 programBank;
  n2 characterBank;
  n1 nametableBank;
};
