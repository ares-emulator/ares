struct HVC_TxROM : Interface {  //MMC3
  static auto create(string id) -> Interface* {
    if(id == "HVC-TBROM" ) return new HVC_TxROM(Revision::TBROM);
    if(id == "HVC-TEROM" ) return new HVC_TxROM(Revision::TEROM);
    if(id == "HVC-TFROM" ) return new HVC_TxROM(Revision::TFROM);
    if(id == "HVC-TGROM" ) return new HVC_TxROM(Revision::TGROM);
    if(id == "HVC-TKROM" ) return new HVC_TxROM(Revision::TKROM);
    if(id == "HVC-TKSROM") return new HVC_TxROM(Revision::TKSROM);
    if(id == "HVC-TLROM" ) return new HVC_TxROM(Revision::TLROM);
    if(id == "HVC-TL1ROM") return new HVC_TxROM(Revision::TL1ROM);
    if(id == "HVC-TL2ROM") return new HVC_TxROM(Revision::TL2ROM);
    if(id == "HVC-TLSROM") return new HVC_TxROM(Revision::TLSROM);
    if(id == "HVC-TNROM" ) return new HVC_TxROM(Revision::TNROM);
    if(id == "HVC-TQROM" ) return new HVC_TxROM(Revision::TQROM);
    if(id == "HVC-TR1ROM") return new HVC_TxROM(Revision::TR1ROM);
    if(id == "HVC-TSROM" ) return new HVC_TxROM(Revision::TSROM);
    if(id == "HVC-TVROM" ) return new HVC_TxROM(Revision::TVROM);
    if(id == "NES-QJ"    ) return new HVC_TxROM(Revision::NESQJ);
    if(id == "PAL-ZZ"    ) return new HVC_TxROM(Revision::PALZZ);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
    TBROM,
    TEROM,
    TFROM,
    TGROM,
    TKROM,
    TKSROM,
    TLROM,
    TL1ROM,
    TL2ROM,
    TLSROM,
    TNROM,
    TQROM,
    TR1ROM,
    TSROM,
    TVROM,
    NESQJ,
    PALZZ,
  } revision;

  HVC_TxROM(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
  }

  auto main() -> void override {
    if(irqDelay) irqDelay--;
    cpu.irqLine(irqLine);
  }

  auto irqTest(n16 address) -> void {
    if(!(characterAddress & 0x1000) && (address & 0x1000)) {
      if(irqDelay == 0) {
        if(irqCounter == 0) {
          irqCounter = irqLatch + 1;
        }
        if(--irqCounter == 0) {
          if(irqEnable) irqLine = 1;
        }
      }
      irqDelay = 6;
    }
    characterAddress = address;
  }

  inline auto bankPRG(n32 address) -> n6 {
    switch(address >> 13 & 3) {
    case 0: return (programMode == 0 ? programBank[0] : (n6)0x3e); break;
    case 1: return programBank[1]; break;
    case 2: return (programMode == 1 ? programBank[0] : (n6)0x3e); break;
    case 3: default: return 0x3f; break;
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;

    if(address < 0x8000) {
      if(!ramEnable || !programRAM) return data;
      return programRAM.read((n13)address);
    }

    n6 bank = bankPRG(address);
    address = bank << 13 | (n13)address;
    if(revision == Revision::NESQJ) {
      address = outerBank.bit(0) << 17 | (n17)address;
    }
    if(revision == Revision::PALZZ) {
      n1 a16 = (address.bit(16) & outerBank.bit(2)) | (outerBank.bit(1) & outerBank.bit(0));
      address = outerBank.bit(2) << 17 | a16 << 16 | (n16)address;
    }
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address == 0x2006 && ++characterLatch) irqTest(data << 8);
    if(address < 0x6000) return;

    if(address < 0x8000) {
      if(!ramEnable || !ramWritable) return;
      if(revision == Revision::NESQJ) outerBank = data.bit(0);
      if(revision == Revision::PALZZ) outerBank = data.bit(0,2);
      if(!programRAM) return;
      return programRAM.write((n13)address, data);
    }

    switch(address & 0xe001) {
    case 0x8000:
      bankSelect = data.bit(0,2);
      programMode = data.bit(6);
      characterMode = data.bit(7);
      break;
    case 0x8001:
      switch(bankSelect) {
      case 0: characterBank[0] = data & ~1; break;
      case 1: characterBank[1] = data & ~1; break;
      case 2: characterBank[2] = data; break;
      case 3: characterBank[3] = data; break;
      case 4: characterBank[4] = data; break;
      case 5: characterBank[5] = data; break;
      case 6: programBank[0] = data.bit(0,5); break;
      case 7: programBank[1] = data.bit(0,5); break;
      }
      break;
    case 0xa000:
      mirror = data.bit(0);
      break;
    case 0xa001:
      ramWritable = !data.bit(6);
      ramEnable = data.bit(7);
      break;
    case 0xc000:
      irqLatch = data;
      break;
    case 0xc001:
      irqCounter = 0;
      break;
    case 0xe000:
      irqEnable = 0;
      irqLine = 0;
      break;
    case 0xe001:
      irqEnable = 1;
      break;
    }
  }

  auto debugAddress(n32 address) -> n32 override {
    if(address < 0x8000) return address;
    return (bankPRG(address) << 16 | (n16)address) & ((bit::round(programROM.size()) << 2) - 1);
  }

  auto addressCHR(n32 address) const -> n32 {
    if(characterMode == 0) {
      if(address <= 0x07ff) return characterBank[0] << 10 | (n11)address;
      if(address <= 0x0fff) return characterBank[1] << 10 | (n11)address;
      if(address <= 0x13ff) return characterBank[2] << 10 | (n10)address;
      if(address <= 0x17ff) return characterBank[3] << 10 | (n10)address;
      if(address <= 0x1bff) return characterBank[4] << 10 | (n10)address;
      if(address <= 0x1fff) return characterBank[5] << 10 | (n10)address;
    } else {
      if(address <= 0x03ff) return characterBank[2] << 10 | (n10)address;
      if(address <= 0x07ff) return characterBank[3] << 10 | (n10)address;
      if(address <= 0x0bff) return characterBank[4] << 10 | (n10)address;
      if(address <= 0x0fff) return characterBank[5] << 10 | (n10)address;
      if(address <= 0x17ff) return characterBank[0] << 10 | (n11)address;
      if(address <= 0x1fff) return characterBank[1] << 10 | (n11)address;
    }
    unreachable;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::TKSROM || revision == Revision::TLSROM) {
      address.bit(12) = 0;
      if(characterMode == 0) {
        if(address <= 0x27ff) return characterBank[0].bit(7) << 10 | (n10)address;
        if(address <= 0x2fff) return characterBank[1].bit(7) << 10 | (n10)address;
      } else {
        if(address <= 0x23ff) return characterBank[2].bit(7) << 10 | (n10)address;
        if(address <= 0x27ff) return characterBank[3].bit(7) << 10 | (n10)address;
        if(address <= 0x2bff) return characterBank[4].bit(7) << 10 | (n10)address;
        if(address <= 0x2fff) return characterBank[5].bit(7) << 10 | (n10)address;
      }
      unreachable;
    }
    return address >> mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    irqTest(address);
    if(revision == Revision::TVROM) {
      if(address < 0x2000) return characterROM.read(addressCHR(address));
      address.bit(12) = 0;
      if(address < 0x2800) return ppu.readCIRAM((n11)address);
      return characterRAM.read((n11)address);
    }
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(revision == Revision::TQROM) {
      address = addressCHR(address);
      if(address < 0x10000) return characterROM.read(address);
      return characterRAM.read(address);
    }
    if(revision == Revision::NESQJ) {
      address = outerBank.bit(0) << 17 | (n17)addressCHR(address);
      return characterROM.read(address);
    }
    if(revision == Revision::PALZZ) {
      address = outerBank.bit(2) << 17 | (n17)addressCHR(address);
      return characterROM.read(address);
    }
    if(characterRAM) return characterRAM.read(addressCHR(address));
    return characterROM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    irqTest(address);
    if(revision == Revision::TVROM) {
      if(address < 0x2000) return;
      address.bit(12) = 0;
      if(address < 0x2800) return ppu.writeCIRAM((n11)address, data);
      return characterRAM.write((n11)address, data);
    }
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(revision == Revision::TQROM) {
      address = addressCHR(address);
      if(address < 0x10000) return;
      return characterRAM.write(address, data);
    }
    if(characterRAM) return characterRAM.write(addressCHR(address), data);
  }

  auto power() -> void override {
    outerBank = 0;
    ramEnable = 1;
    ramWritable = 1;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(characterMode);
    s(programMode);
    s(bankSelect);
    s(programBank);
    s(characterBank);
    s(outerBank);
    s(mirror);
    s(ramEnable);
    s(ramWritable);
    s(irqLatch);
    s(irqCounter);
    s(irqEnable);
    s(irqDelay);
    s(irqLine);
    s(characterLatch);
    s(characterAddress);
  }

  n1  characterMode;
  n1  programMode;
  n3  bankSelect;
  n6  programBank[2];
  n8  characterBank[6];
  n3  outerBank;
  n1  mirror;
  n1  ramEnable;
  n1  ramWritable;
  n8  irqLatch;
  n8  irqCounter;
  n1  irqEnable;
  n8  irqDelay;
  n1  irqLine;
  n1  characterLatch;
  n16 characterAddress;
};
