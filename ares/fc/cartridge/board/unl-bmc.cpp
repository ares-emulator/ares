struct BMC : Interface {
  static auto create(string id) -> Interface* {
    if (id == "UNL-BMC-32") return new BMC(Revision::BMC_32);
    if (id == "UNL-BMC-128") return new BMC(Revision::BMC_128);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    BMC_32,  // 229: max 32 * 16KB PRG-ROM, 32 * 8KB CHR-ROM
    BMC_128, // 225/255: max 128 * 16KB PRG-ROM, 128 * 8KB CHR-ROM
  } revision;

  BMC(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if (address >= 0x5800 && address < 0x6000 && revision == Revision::BMC_128) {
      return regs[address & 3];
    }
    if (address < 0x8000) return data;

    if (mode == 1)
      address = prgBank << 15 | (n15)address;
    else
      address = prgBank << 14 | (n14)address;

    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if (address >= 0x5800 && address < 0x6000 && revision == Revision::BMC_128) {
      regs[address & 3] = data;
      return;
    }
    if (address < 0x8000) return;

    update(address);
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
    update(0x8000);

    if (revision == Revision::BMC_128)
      for (auto& reg : regs) reg = random();
  }

  auto serialize(serializer& s) -> void override {
    s(latch);
    update(latch);

    if (revision == Revision::BMC_128) s(regs);
  }

  auto update(n16 address) -> void {
    latch = address;
    
    if (revision == Revision::BMC_32) {
      mode    = address.bit(1, 4) == 0;
      chrBank = address.bit(0, 4);
      prgBank = address.bit(0, 4);
      mirror  = address.bit(5);
    } else {
      mirror    = latch.bit(13);
      mode      = latch.bit(12) == 0;
      chrBank   = latch.bit(0, 5)  | latch.bit(14) << 6;
      prgBank   = latch.bit(6, 11) | latch.bit(14) << 6;
    }

    if (mode == 1)
      prgBank >>= 1;
  }

  n16 latch;

  n1 mode;
  n1 mirror;
  n7 chrBank;
  n7 prgBank;

  n4 regs[4];
};
