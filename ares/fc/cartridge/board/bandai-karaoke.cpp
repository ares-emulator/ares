struct BandaiKaraoke : Interface {
  static auto create(string id) -> Interface* {
    if(id == "BANDAI-KARAOKE") return new BandaiKaraoke;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> optionROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(optionROM, "option.rom");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) {
      data.bit(0) = 1; //A button
      data.bit(1) = 1; //B button
      //data.bit(2) = 1; //1-bit ADC
      return data;
    }

    if(address < 0xc000) {
      if(!optionEnable) return programROM.read(programBank << 14 | (n14)address);
      if(optionROM) return optionROM.read(programBank << 14 | (n14)address);
      return data;
    }
    return programROM.read(0xf << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank = data.bit(0,3);
    optionEnable = !data.bit(4);
    mirror = data.bit(5);
  }

  auto addressCIRAM(n32 address) -> n32 {
    return address >> mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterRAM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    return characterRAM.write(address, data);
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(programBank);
    s(optionEnable);
    s(mirror);
  }

  n4 programBank;
  n1 optionEnable;
  n1 mirror;
};
