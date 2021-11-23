struct HVC_CNROM : Interface {
  static auto create(string id) -> Interface* {
    if(id == "HVC-CNROM"    ) return new HVC_CNROM(Revision::CNROM);
    if(id == "HVC-CNROM-SEC") return new HVC_CNROM(Revision::CNROMS);
    if(id == "HVC-CPROM"    ) return new HVC_CNROM(Revision::CPROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
    CNROM,
    CNROMS,
    CPROM,
  } revision;

  HVC_CNROM(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
    key = pak->attribute("chip/key").natural();
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read((n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    characterBank = data.bit(0,1);
    characterEnable = (data == key);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    if(revision == Revision::CPROM) {
      n2 bank = (address < 0x1000) ? (n2)0 : characterBank;
      return characterRAM.read(bank << 12 | (n12)address);
    }
    if(revision == Revision::CNROMS) {
      if(!characterEnable) return 0xff;
      return characterROM.read((n13)address);
    }
    address = characterBank << 13 | (n13)address;
    return characterROM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
    if(revision == Revision::CPROM) {
      n2 bank = (address < 0x1000) ? (n2)0 : characterBank;
      return characterRAM.write(bank << 12 | (n12)address, data);
    }
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(mirror);
    s(key);
    s(characterBank);
    s(characterEnable);
  }

  n1 mirror;  //0 = horizontal, 1 = vertical
  n8 key;
  n2 characterBank;
  n1 characterEnable;
};
