struct BMC31In1 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "UNL-BMC-31-IN-1") return new BMC31In1;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if (address < 0x8000) return data;
    if (prgMode == 1)
      address = (n15)address;
    else
      address = prgBank << 14 | (n14)address;

    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if (address < 0x8000) return;

    chrBank = address.bit(0, 4);
    prgMode = (address & 0x1e) == 0;
    prgBank = address.bit(0, 4);
    mirror = address.bit(5);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if (address & 0x2000) {
      address = (address >> mirror & 0x0400) | (n10)address;
      return ppu.readCIRAM(address);
    }

    return characterROM.read(chrBank << 13 | (n13)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if (address & 0x2000) {
      address = (address >> mirror & 0x0400) | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto power() -> void override {
    writePRG(0x8000, 0);
  }

  n1 mirror;
  n1 prgMode;
  n5 prgBank;
  n8 chrBank;
};
