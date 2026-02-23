struct AveNina001 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "AVE-NINA-001") return new AveNina001;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;

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
    if(address < 0x8000) return programRAM.read((n13)address);
    return programROM.read(programBank << 15 | (n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) {
      switch(address) {
      case 0x7ffd: programBank = data.bit(0); break;
      case 0x7ffe: characterBank[0] = data.bit(0,3); break;
      case 0x7fff: characterBank[1] = data.bit(0,3); break;
      }
      return programRAM.write((n13)address, data);
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM((n11)address);
    n4 bank = characterBank[address.bit(12)];
    return characterROM.read(bank << 12 | (n12)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM((n11)address, data);
  }

  auto power() -> void override {
    programBank = 0;
    characterBank[0] = 0;
    characterBank[1] = 1;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(programBank);
    s(characterBank);
  }

  n1 programBank;
  n4 characterBank[2];
};
