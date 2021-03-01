struct HVC_DxROM : Interface {
  static auto create(string id) -> Interface* {
    if(id == "HVC-DEROM" ) return new HVC_DxROM(Revision::DEROM);
    if(id == "HVC-DE1ROM") return new HVC_DxROM(Revision::DE1ROM);
    if(id == "HVC-DRROM" ) return new HVC_DxROM(Revision::DRROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterCIRAM;

  enum class Revision : u32 {
    DEROM,
    DE1ROM,
    DRROM,
  } revision;

  HVC_DxROM(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterCIRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(characterCIRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    switch(address) {
    case 0x8000 ... 0x9fff: return programROM.read(programBank[0] << 13 | (n13)address);
    case 0xa000 ... 0xbfff: return programROM.read(programBank[1] << 13 | (n13)address);
    case 0xc000 ... 0xdfff: return programROM.read(0x0e << 13 | (n13)address);
    case 0xe000 ... 0xffff: return programROM.read(0x0f << 13 | (n13)address);
    }
    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    switch(address & 0xe001) {
    case 0x8000:
      bankSelect = data.bit(0,2);
      break;
    case 0x8001:
      switch(bankSelect) {
      case 0: characterBank[0] = data & ~1; break;
      case 1: characterBank[1] = data & ~1; break;
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

  auto readCHR(n32 address, n8 data) -> n8 override {
    switch(address) {
    case 0x0000 ... 0x07ff: return characterROM.read(characterBank[0] << 10 | (n11)address);
    case 0x0800 ... 0x0fff: return characterROM.read(characterBank[1] << 10 | (n11)address);
    case 0x1000 ... 0x13ff: return characterROM.read(characterBank[2] << 10 | (n10)address);
    case 0x1400 ... 0x17ff: return characterROM.read(characterBank[3] << 10 | (n10)address);
    case 0x1800 ... 0x1bff: return characterROM.read(characterBank[4] << 10 | (n10)address);
    case 0x1c00 ... 0x1fff: return characterROM.read(characterBank[5] << 10 | (n10)address);
    case 0x2000 ... 0x27ff: return ppu.readCIRAM((n11)address);
    case 0x2800 ... 0x2fff: return characterCIRAM.read((n11)address);
    }
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    switch(address) {
    case 0x2000 ... 0x27ff: return ppu.writeCIRAM((n11)address, data);
    case 0x2800 ... 0x2fff: return characterCIRAM.write((n11)address, data);
    }
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(bankSelect);
    s(programBank);
    s(characterBank);
  }

  n3 bankSelect;
  n4 programBank[2];
  n6 characterBank[6];
};
