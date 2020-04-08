struct HVC_AxROM : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;

  enum class Revision : uint {
    AMROM,
    ANROM,
    AN1ROM,
    AOROM,
  } revision;

  HVC_AxROM(Markup::Node document, Revision revision) : Interface(document), revision(revision) {}

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x8000) return cpu.mdr();
    return programROM.read(programBank << 15 | (uint15)address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x8000) return;
    programBank = data.bit(0,3);
    mirror = data.bit(4);
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) return ppu.readCIRAM(mirror << 10 | address & 0x03ff);
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return 0x00;
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) return ppu.writeCIRAM(mirror << 10 | address & 0x03ff, data);
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void {
    programBank = 0x0f;
    mirror = 0;
  }

  auto serialize(serializer& s) -> void {
    characterRAM.serialize(s);
    s.integer(programBank);
    s.integer(mirror);
  }

  uint4 programBank;
  uint1 mirror;
};
