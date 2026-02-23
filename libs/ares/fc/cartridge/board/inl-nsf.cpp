struct INL_NSF : Interface {
  static auto create(string id) -> Interface* {
    if(id == "INL-NSF") return new INL_NSF;
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
    return programROM.read((banks[(address >> 12) & 7] << 12) | (n12)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if((address & 0xF000) != 0x5000) return;
    banks[address & 7] = data;
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
    banks[7] = 0xFF;
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(mirror);
    s(banks);
  }

  n1 mirror;  //0 = horizontal, 1 = vertical
  n8 banks[8];
};
