struct JalecoJF05 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "JALECO-JF-05") return new JalecoJF05;
    if(id == "JALECO-JF-06") return new JalecoJF05;
    if(id == "JALECO-JF-07") return new JalecoJF05;
    if(id == "JALECO-JF-08") return new JalecoJF05;
    if(id == "JALECO-JF-10") return new JalecoJF05;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000 || address > 0x7fff) return;
    characterBank = data.bit(0) << 1 | data.bit(1);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    address = characterBank << 13 | (n13)address;
    return characterROM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto serialize(serializer& s) -> void override {
    s(mirror);
    s(characterBank);
  }

  n1 mirror;
  n2 characterBank;
};
