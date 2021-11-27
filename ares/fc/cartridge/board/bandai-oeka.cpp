struct BandaiOeka : Interface {
  static auto create(string id) -> Interface* {
    if(id == "BANDAI-OEKA") return new BandaiOeka;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto addressTest(n32 address) -> void {
    if((characterAddress >> 12) != 2 && (address >> 12) == 2) {
      characterBank.bit(0,1) = address >> 8 & 3;
    }
    characterAddress = address;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(programBank << 15 | (n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address == 0x2006) {
      if(++characterLatch) addressTest(data << 8);
    }
    if(address < 0x8000) return;
    programBank = data.bit(0,1);
    characterBank.bit(2) = data.bit(2);
  }

  auto addressCHR(n32 address) const -> n32 {
    n3 bank = (address < 0x1000) ? characterBank : n3(characterBank & 4 | 3);
    return bank << 12 | (n12)address;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    return address >> !mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    addressTest(address);
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterRAM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    addressTest(address);
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    return characterRAM.write(addressCHR(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(mirror);
    s(programBank);
    s(characterBank);
    s(characterLatch);
    s(characterAddress);
  }

  n1  mirror;
  n2  programBank;
  n3  characterBank;
  n1  characterLatch;
  n32 characterAddress;
};
