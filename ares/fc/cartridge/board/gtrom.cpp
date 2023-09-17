struct GTROM : Interface {
  static auto create(string id) -> Interface* {
    if(id == "GTROM") return new GTROM();
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> characterRAM;
  Memory::Writable<n8> videoRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterRAM, "character.ram");
    videoRAM.allocate(16_KiB);
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data; 
    return programROM.read(programBank << 15 | (n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x5000 && address <= 0x5fff) {
      programBank = data.bit(0, 3);
      characterBank = data.bit(4);
      videoBank = data.bit(5);
      return;
    }
  }

  auto debugAddress(n32 address) -> n32 override {
    if(address < 0x8000) return address;
    return (programBank << 16 | (n16)address) & ((bit::round(programROM.size()) << 1) - 1);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return videoRAM.read(videoBank << 13 | (n13)address);
    if(characterRAM) return characterRAM.read(characterBank << 13 | (n13)address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return videoRAM.write(videoBank << 13 | (n13)address, data);
    return characterRAM.write(characterBank << 13 | (n13)address, data);
  }

  auto power() -> void override {
    programBank = 0;
    characterBank = 0;
    videoBank = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(videoRAM);
    s(programBank);
    s(characterBank);
    s(videoBank);
  }

  n4 programBank;
  n1 characterBank;
  n1 videoBank;
};
