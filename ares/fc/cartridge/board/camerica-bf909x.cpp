struct Camerica_BF909x : Interface {
  static auto create(string id) -> Interface* {
    if(id == "CAMERICA-BF9093" ) return new Camerica_BF909x(Revision::CAMERICA_BF9093);
    if(id == "CAMERICA-BF9096" ) return new Camerica_BF909x(Revision::CAMERICA_BF9096);
    if(id == "CAMERICA-BF9096A") return new Camerica_BF909x(Revision::CAMERICA_BF9096A);
    if(id == "CAMERICA-BF9097" ) return new Camerica_BF909x(Revision::CAMERICA_BF9097);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
    CAMERICA_BF9093,
    CAMERICA_BF9096,
    CAMERICA_BF9096A,
    CAMERICA_BF9097,
  } revision;

  Camerica_BF909x(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    n4 bank;
    if(address < 0xC000) {
      bank = programBank;
    } else {
      bank = 0x0f;
    }
    if(revision == Revision::CAMERICA_BF9096 || revision == Revision::CAMERICA_BF9096A) {
      bank.bit(2,3) = programOuterBank;
    }
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    if(address < 0xC000) {
      if(revision == Revision::CAMERICA_BF9096) {
        programOuterBank = data.bit(3,4);
      } else if(revision == Revision::CAMERICA_BF9096A) {
        programOuterBank.bit(0) = data.bit(4);
        programOuterBank.bit(1) = data.bit(3);
      } else if(address < 0xA000 && revision == Revision::CAMERICA_BF9097) {
        characterBank = data.bit(4);
      }
    } else {
      programBank = data.bit(0,3);
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::CAMERICA_BF9097) {
      return characterBank << 10 | (n10)address;
    } else {
      return address >> !mirror & 0x0400 | (n10)address;
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      return ppu.readCIRAM(addressCIRAM(address));
    }
    return characterRAM.read((n13)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      return ppu.writeCIRAM(addressCIRAM(address), data);
    }
    return characterRAM.write((n13)address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(mirror);
    s(programBank);
    s(programOuterBank);
    s(characterBank);
  }

  n1 mirror;  //0 = horizontal, 1 = vertical
  n4 programBank;
  n2 programOuterBank;
  n1 characterBank;
};
