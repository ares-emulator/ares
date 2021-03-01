struct IremTAMS1 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "IREM-TAM-S1") return new IremTAMS1;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n6 bank, banks = programROM.size() >> 14;
    switch(address & 0xc000) {
    case 0x8000: bank = banks - 1; break;
    case 0xc000: bank = programBank; break;
    }
    address = bank << 14 | (n14)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank = data.bit(0,5);
    mirror      = data.bit(6,7);
  }

  auto addressCIRAM(n32 address) const -> n32 {
    switch(mirror) {
    case 0: return 0x0000 | address & 0x03ff;                 //one-screen mirroring (first)
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal mirroring
    case 2: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical mirroring
    case 3: return 0x0400 | address & 0x03ff;                 //one-screen mirroring (second)
    }
    unreachable;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(programBank);
    s(mirror);
  }

  n6 programBank;
  n2 mirror;
};
