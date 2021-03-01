struct HVC_NROM : Interface {
  static auto create(string id) -> Interface* {
    if(id == "HVC-NROM"    ) return new HVC_NROM;
    if(id == "HVC-NROM-128") return new HVC_NROM;
    if(id == "HVC-NROM-256") return new HVC_NROM;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000 && !programRAM) return data;
    if(address < 0x8000) return programRAM.read(address);
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000 && !programRAM) return;
    if(address < 0x8000) return programRAM.write(address, data);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> !mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(mirror);
  }

  n1 mirror;  //0 = horizontal, 1 = vertical
};
