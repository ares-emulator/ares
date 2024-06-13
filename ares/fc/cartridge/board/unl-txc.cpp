// TXC-22211B: Mapper 172
//   1991 賭馬 Racing (1991 Dǔmǎ Racing, "Enjoyable Horse Racing")
//   麻将方块 (Mahjong Block) (original release)
//   Venice Beach Volley (Super Mega release)

struct TXC : Interface {
  static auto create(string id) -> Interface* {
    // if (id == "TXC-22211A") return new TXC(Revision::TXC_22211A);
    if (id == "TXC-22211B") return new TXC(Revision::TXC_22211B);
    // if (id == "TXC-22211C") return new TXC(Revision::TXC_22211C);
    return nullptr;
  }

  enum class Revision : u32 {
    // TXC_22211A,
    TXC_22211B,
    // TXC_22211C,
  } revision;

  TXC(Revision revision) : revision(revision) {}

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if (address >= 0x4100 && address < 0x4104 && revision == Revision::TXC_22211B) {
      data = data & 0xc0 | bit::reverse(jv001.read()) >> 2;
      chrBank = jv001.out.bit(0, 1);
      mirror = jv001.invert;
      return data;
    }
    if (address < 0x8000) return data;

    return programROM.read((n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if (revision == Revision::TXC_22211B)
      return jv001.write(address, bit::reverse(data) >> 2);
    if (address < 0x8000) return;

    chrBank = jv001.out.bit(0, 1);
    mirror = jv001.invert;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if (address & 0x2000) {
      address = (address >> mirror & 0x0400) | (n10)address;
      return ppu.readCIRAM(address);
    }

    return characterROM.read(chrBank << 13 | (n13)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if (address & 0x2000) {
      address = (address >> mirror & 0x0400) | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto power() -> void override {
    jv001 = {};
    mirror = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(jv001);
    s(chrBank);
    s(mirror);
  }

  JV001 jv001;
  n2 chrBank;
  n1 mirror;
};
