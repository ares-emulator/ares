struct IremLROG017 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "IREM-LROG017") return new IremLROG017;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
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
    return programROM.read(programBank << 15 | (n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank   = data.bit(0,3);
    characterBank = data.bit(4,7);
  }

  auto addressCHR(n32 address) const -> n32 {
    if(address < 0x1000) return 1 << 11 | (n11)address;
    if(address < 0x1800) return 2 << 11 | (n11)address;
    if(address < 0x2000) return 3 << 11 | (n11)address;
    if(address < 0x2800) return 0 << 11 | (n11)address;
    unreachable;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address < 0x0800) return characterROM.read(characterBank << 11 | (n11)address);
    if(address < 0x2800) return characterRAM.read(addressCHR(address));
    if(address < 0x3000) return ppu.readCIRAM((n11)address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address < 0x0800) return;
    if(address < 0x2800) return characterRAM.write(addressCHR(address), data);
    if(address < 0x3000) return ppu.writeCIRAM((n11)address, data);
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(programBank);
    s(characterBank);
  }

  n4 programBank;
  n4 characterBank;
};
