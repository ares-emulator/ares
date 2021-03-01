struct Sunsoft2 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "SUNSOFT-2") return new Sunsoft2;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    staticMirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n3 bank;
    switch(address & 0xc000) {
    case 0x8000: bank = programBank; break;
    case 0xc000: bank = ~0; break;
    }
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address & 0x8000) {
      characterBank.bit(0,2) = data.bit(0,2);
      mirror = data.bit(3);
      programBank = data.bit(4,6);
      characterBank.bit(3) = data.bit(7);
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(characterROM) return mirror << 10 | (n10)address;
    if(characterRAM) return address >> !staticMirror & 0x0400 | address & 0x03ff;
    return 0;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(characterBank << 13 | (n13)address);
    if(characterRAM) return characterRAM.read(characterBank << 13 | (n13)address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(characterBank << 13 | (n13)address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
    s(mirror);
  }

  n3 programBank;
  n4 characterBank;
  n1 mirror;
  n1 staticMirror;
};
