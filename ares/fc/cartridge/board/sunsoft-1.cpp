struct Sunsoft1 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "SUNSOFT-1") return new Sunsoft1;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x6000 && address <= 0x7fff) {
      characterBank[0] = data.bit(0,2);
      characterBank[1] = data.bit(4,5) | 4;
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | address & 0x03ff;
      return ppu.readCIRAM(address);
    }
    n3 bank;
    switch(address & 0x1000) {
    case 0x0000: bank = characterBank[0]; break;
    case 0x1000: bank = characterBank[1]; break;
    }
    if(characterROM) return characterROM.read(bank << 12 | (n12)address);
    if(characterRAM) return characterRAM.read(bank << 12 | (n12)address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | address & 0x03ff;
      return ppu.writeCIRAM(address, data);
    }
    n3 bank;
    switch(address & 0x1000) {
    case 0x0000: bank = characterBank[0]; break;
    case 0x1000: bank = characterBank[1]; break;
    }
    if(characterRAM) return characterRAM.write(bank << 12 | (n12)address, data);
  }

  auto power() -> void override {
    characterBank[0] = 0;
    characterBank[1] = 4;
  }

  auto serialize(serializer& s) -> void override {
    s(characterBank);
  }

  n3 characterBank[2];
  n1 mirror;
};
