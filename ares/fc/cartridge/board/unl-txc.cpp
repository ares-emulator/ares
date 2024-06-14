// TXC-22211A: Mapper 132
// TXC-22211B: Mapper 172

struct TXC : Interface {
  static auto create(string id) -> Interface*;

  enum class Revision : u32 {
    TXC_22211A,
    TXC_22211B,
    TXC_22211C,
  } revision;

  TXC(Revision revision) : revision(revision) {}

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }
};

struct TXC_22211AC : TXC {
  TXC_22211AC(Revision revision) : TXC(revision) {}

  auto readPRG(n32 address, n8 data) -> n8 override {
    if ((address & 0xe100) == 0x4100) {
      data &= 0xf0;
      data.bit(0,2) = txc.reg.bit(0,2);
      data.bit(3)   = txc.reg.bit(4);
      return data;
    }
    if (address < 0x8000) return data;

    if (revision == Revision::TXC_22211A)
      address = txc.out.bit(2) << 15 | (n15)address;

    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if ((address & 0xe100) == 0x4100) {
      txc.writePRG(address, data.bit(3) << 4 | data.bit(0,2));
    }
    if (address < 0x8000) return;

    txc.writePRG(address, data.bit(3) << 4 | data.bit(0,2));
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if (address & 0x2000) return ppu.readCIRAM((n11)address);

    if (revision == Revision::TXC_22211A)
      address = txc.out.bit(0,1) << 13 | (n13)address;
    else
      address = txc.out.bit(1) << 15 | txc.o3 << 14 | txc.out.bit(0) << 13 | (n13)address;

    return characterROM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if (address & 0x2000) return ppu.writeCIRAM((n11)address, data);
  }

  auto power() -> void override {
    txc = {};
    txc.overdown = false;
    txc.i0 = 1;
    txc.i1 = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(txc);
  }

  TXC05_00002_010 txc;
};

struct TXC_22211B : TXC {
  TXC_22211B() : TXC(Revision::TXC_22211B) {}

  auto readPRG(n32 address, n8 data) -> n8 override {
    if ((address & 0xe100) == 0x4100) {
      n8 v = (jv001.reg.bit(4,5) ^ (jv001.invert ? 3 : 0)) << 4;
      v |= jv001.reg.bit(0,3);
      return data & 0xc0 | bit::reverse(v) >> 2;
    }
    if (address < 0x8000) return data;

    return programROM.read((n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if ((address & 0xe100) == 0x4100)
      return jv001.writePRG(address, bit::reverse(data) >> 2);
    if (address < 0x8000) return;

    return jv001.writePRG(address, bit::reverse(data) >> 2);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if (address & 0x2000) {
      address = (address >> !jv001.invert & 0x0400) | (n10)address;
      return ppu.readCIRAM(address);
    }
    return characterROM.read(jv001.out.bit(0,1) << 13 | (n13)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if (address & 0x2000) {
      address = (address >> !jv001.invert & 0x0400) | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto power() -> void override {
    jv001 = {};
  }

  auto serialize(serializer& s) -> void override {
    s(jv001);
  }

  JV001 jv001;
};

auto TXC::create(string id) -> Interface* {
    if (id == "UNL-22211" ) return new TXC_22211AC(Revision::TXC_22211A);
    if (id == "TXC-22211A") return new TXC_22211AC(Revision::TXC_22211A);
    if (id == "TXC-22211B") return new TXC_22211B();
    if (id == "TXC-22211C") return new TXC_22211AC(Revision::TXC_22211C);
    return nullptr;
}
