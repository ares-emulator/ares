struct JYCompany : Interface {
  static auto create(string id) -> Interface* {
    if(id == "UNL-JY-COMPANY-A") return new JYCompany(Revision::JYTypeA);
    if(id == "UNL-JY-COMPANY-B") return new JYCompany(Revision::JYTypeB);
    if(id == "UNL-JY-COMPANY-C") return new JYCompany(Revision::JYTypeC);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    JYTypeA,
    JYTypeB,
    JYTypeC,
  } revision;

  enum class IrqSource : u32 {
    CpuM2Rise  = 0,
    PpuA12Rise = 1,  // unfiltered, eight per scanline
    PpuRead    = 2,  // 170 per scanline ?
    CpuWrite   = 3,
  };

  JYCompany(Revision revision) : revision(revision) {
  }

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto main() -> void override {
    if(irqSource == IrqSource::CpuM2Rise) tickIrqCounter();
    if(irqSource == IrqSource::CpuWrite && cpu.isWriting()) tickIrqCounter();
    cpu.irqLine(irqLine);
    tick();
  }

  auto updatePRGBanks() -> void {
    n8 exPrg = exPrgMode << 6;
    n8 last  = (prgMode & 0x04) ? prgRegs[3] : n8(0x3f);

    switch(prgMode & 0x03) {
    case 0:  // 32KB
      programBanks[0]   = ((last & 0x0f) | (exPrg >> 2)) << 2;
      programBanks[1]   = programBanks[0] + 1;
      programBanks[2]   = programBanks[0] + 2;
      programBanks[3]   = programBanks[0] + 3;
      programAt6000Bank = (((prgRegs[3] * 4) + 3) & 0x3f) | (exPrg >> 2);
      break;
    case 1:  // 16KB
      programBanks[0]   = ((prgRegs[1] & 0x1f) | (exPrg >> 1)) << 1;
      programBanks[1]   = programBanks[0] + 1;
      programBanks[2]   = ((last & 0x1f) | (exPrg >> 1)) << 1;
      programBanks[3]   = programBanks[2] + 1;
      programAt6000Bank = (((prgRegs[3] * 2) + 1) & 0x1f) | (exPrg >> 1);
      break;
    case 2:  // 8KB
      programBanks[0]   = prgRegs[0] | exPrg;
      programBanks[1]   = prgRegs[1] | exPrg;
      programBanks[2]   = prgRegs[2] | exPrg;
      programBanks[3]   = last | exPrg;
      programAt6000Bank = prgRegs[3] | exPrg;
      break;
    case 3:  // 8KB Alt (Scrambled)
      programBanks[0] = (invertPrgBits(prgRegs[0]) & 0x3f) | exPrg;
      programBanks[1] = (invertPrgBits(prgRegs[1]) & 0x3f) | exPrg;
      programBanks[2] = (invertPrgBits(prgRegs[2]) & 0x3f) | exPrg;
      if(prgMode & 0x04)
        programBanks[3] = (invertPrgBits(prgRegs[3]) & 0x3f) | exPrg;
      else
        programBanks[3] = (invertPrgBits(last) & 0x3f) | exPrg;
      programAt6000Bank = (invertPrgBits(prgRegs[3]) & 0x3f) | exPrg;
      break;
    }
  }

  // PRG read
  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x5000) return data;
    // multiply/accumulator registers at $5000-$5FFF
    if(address >= 0x5000 && address < 0x6000) {
      if(address < 0x5800) { return data & 0x3f; }
      switch(address & 0x07) {
      case 0: return (multiplyValue1 * multiplyValue2) & 0xff;
      case 1: return ((multiplyValue1 * multiplyValue2) >> 8) & 0xff;
      case 3: return regRamValue;
      }
      return data;
    }

    //$6000-$7FFF: optional PRG bank
    if(address >= 0x6000 && address < 0x8000) {
      if(!enablePrgAt6000) return data;
      return programROM.read((u32)programAt6000Bank << 13 | (n13)address);
    }

    //$8000-$FFFF: PRG banking
    if(address >= 0x8000) return programROM.read((u32)programBanks[address >> 13 & 0x03] << 13 | (n13)address);

    unreachable;
  }

  // PRG write
  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x5000) return;

    if(address >= 0x5000 && address < 0x6000) {
      if(address < 0x5800) return;
      switch(address & 0x07) {
      case 0: multiplyValue1 = data; break;
      case 1: multiplyValue2 = data; break;
      case 3: regRamValue = data; break;
      }
      return;
    }

    if(address < 0x8000) return;

    switch(address & 0xf007) {
    case 0x8000:
    case 0x8001:
    case 0x8002:
    case 0x8003:
    case 0x8004:
    case 0x8005:
    case 0x8006:
    case 0x8007:
      prgRegs[address & 0x03] = data & 0x3f;
      updatePRGBanks();
      break;

    case 0x9000:
    case 0x9001:
    case 0x9002:
    case 0x9003:
    case 0x9004:
    case 0x9005:
    case 0x9006:
    case 0x9007:
      chrLowRegs[address & 0x07] = data;
      updateCHRBanks();
      break;

    case 0xa000:
    case 0xa001:
    case 0xa002:
    case 0xa003:
    case 0xa004:
    case 0xa005:
    case 0xa006:
    case 0xa007:
      chrHighRegs[address & 0x07] = data;
      updateCHRBanks();
      break;

    case 0xb000:
    case 0xb001:
    case 0xb002:
    case 0xb003:
      ntLowRegs[address & 0x03] = data;
      updateCIRAMBanks();
      break;

    case 0xb004:
    case 0xb005:
    case 0xb006:
    case 0xb007:
      ntHighRegs[address & 0x03] = data;
      updateCIRAMBanks();
      break;

    case 0xc000:
      if(data & 0x01) {
        irqEnabled = true;
      } else {
        irqEnabled = false;
        irqLine    = 0;
      }
      break;

    case 0xc001:
      irqCountDirection = (data >> 6) & 0x03;
      irqFunkyMode      = (data & 0x08) == 0x08;
      irqSmallPrescaler = ((data >> 2) & 0x01) == 0x01;
      irqSource         = (IrqSource)(data & 0x03);
      break;

    case 0xc002:
      irqEnabled = false;
      irqLine    = 0;
      break;

    case 0xc003: irqEnabled = true; break;
    case 0xc004: irqPrescaler = data ^ irqXorReg; break;
    case 0xc005: irqCounter = data ^ irqXorReg; break;
    case 0xc006: irqXorReg = data; break;
    case 0xc007: irqFunkyModeReg = data; break;

    case 0xd000:
      prgMode           = data & 0x07;
      chrMode           = (data >> 3) & 0x03;
      advancedNtControl = (data & 0x20) == 0x20;
      disableNtRam      = (data & 0x40) == 0x40;
      enablePrgAt6000   = (data & 0x80) == 0x80;

      updatePRGBanks();
      updateCHRBanks();
      updateCIRAMBanks();
      break;

    case 0xd001:
      mirroringReg = data & 0x03;
      updateCIRAMBanks();
      break;

    case 0xd002:
      ntRamSelectBit = data & 0x80;
      updateCIRAMBanks();
      break;

    case 0xd003:
      mirrorChr    = (data & 0x80) == 0x80;
      chrBlockMode = (data & 0x20) == 0x00;
      chrBlock     = (data & 0x01) | ((data & 0x18) >> 2);
      exPrgMode    = (data & 0x06) >> 1;
      updatePRGBanks();
      updateCHRBanks();
      break;
    }
  }

  auto getCHRReg(u32 index) -> n16 {
    if(chrMode >= 2 && mirrorChr) {
      if(index == 2)
        index = 0;
      else if(index == 3)
        index = 1;
    }

    if(chrBlockMode) {
      switch(chrMode) {
      default:
      case 0: return (chrLowRegs[index] & 0x1f) | (chrBlock << 5);
      case 1: return (chrLowRegs[index] & 0x3f) | (chrBlock << 6);
      case 2: return (chrLowRegs[index] & 0x7f) | (chrBlock << 7);
      case 3: return (chrLowRegs[index] & 0xff) | (chrBlock << 8);
      }
    } else {
      return chrLowRegs[index] | (chrHighRegs[index] << 8);
    }
  }

  auto updateCHRBanks() -> void {
    n16 bank;
    switch(chrMode) {
    case 0:
      bank = getCHRReg(0) << 3;
      for(auto b : range(8)) characterBank[0 + b] = bank + b;
      break;

    case 1:
      bank = getCHRReg(chrLatch[0]) << 2;
      for(auto b : range(4)) characterBank[0 + b] = bank + b;

      bank = getCHRReg(chrLatch[1]) << 2;
      for(auto b : range(4)) characterBank[4 + b] = bank + b;
      break;

    case 2:
      bank = getCHRReg(0) << 1;
      for(auto b : range(2)) characterBank[0 + b] = bank + b;

      bank = getCHRReg(2) << 1;
      for(auto b : range(2)) characterBank[2 + b] = bank + b;

      bank = getCHRReg(4) << 1;
      for(auto b : range(2)) characterBank[4 + b] = bank + b;

      bank = getCHRReg(6) << 1;
      for(auto b : range(2)) characterBank[6 + b] = bank + b;
      break;

    case 3:
      for(auto i : range(8)) characterBank[i] = getCHRReg(i);
      break;
    }
  }

  auto updateCIRAMBanks() -> void {
    if(useAdvancedNt()) {
      for(auto i : range(4)) ntBanks[i] = (ntLowRegs[i] & 0x01);
    } else {
      switch(mirroringReg) {
      case 0:
        ntBanks[0] = ntBanks[2] = 0;
        ntBanks[1] = ntBanks[3] = 1;
        break;
      case 1:
        ntBanks[0] = ntBanks[1] = 0;
        ntBanks[2] = ntBanks[3] = 1;
        break;
      case 2: ntBanks[0] = ntBanks[1] = ntBanks[2] = ntBanks[3] = 0; break;
      case 3: ntBanks[0] = ntBanks[1] = ntBanks[2] = ntBanks[3] = 1; break;
      }
    }
  }

  auto addressCHR(n32 address) const -> n32 {
    return (characterBank[address >> 10 & 0x07] << 10) | (n10)address;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    return (ntBanks[(address >> 10) & 0x03] << 10) | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(irqSource == IrqSource::PpuRead && !ppu.isRenderingRead()) tickIrqCounter();
    if(address & 0x2000) {
      if(useAdvancedNt()) {
        n8 ntIndex = (address >> 10) & 0x03;
        if(disableNtRam || (ntLowRegs[ntIndex] & 0x80) != (ntRamSelectBit & 0x80)) {
          // This behavior only affects reads, not writes.
          // Additional info: https://forums.nesdev.com/viewtopic.php?f=3&t=17198
          u16 chrPage = ntLowRegs[ntIndex] | ntHighRegs[ntIndex] << 8;
          return characterROM.read(chrPage << 10 | (n10)address);
        } else {
          return 0;
        }
      }

      return ppu.readCIRAM(addressCIRAM(address));
    }
    return characterROM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto ppuAddressBus(n14 address) -> void override {
    if(irqSource == IrqSource::PpuA12Rise && (address & 0x1000) && !(characterAddress & 0x1000)) tickIrqCounter();
    characterAddress = address;

    if(revision == Revision::JYTypeC) {
      switch(address & 0x0ff0) {
      case 0x0fd0:
        chrLatch[address >> 12 & 0x01] = (address >> 10 & 0x04);
        updateCHRBanks();
        break;
      case 0x0fe0:
        chrLatch[address >> 12 & 0x01] = (address >> 10 & 0x04) | 0x02;
        updateCHRBanks();
        break;
      }
    }
  }

  auto power() -> void override {
    for(auto& r : prgRegs) r = 0;
    exPrgMode         = 0;
    prgMode           = 0;
    enablePrgAt6000   = false;
    programAt6000Bank = 0;
    programBanks[0]   = 0;
    updatePRGBanks();

    for(auto& r : chrLowRegs) r = 0;
    for(auto& r : chrHighRegs) r = 0;
    chrLatch[0]  = 0;
    chrLatch[1]  = 4;
    chrMode      = 0;
    chrBlockMode = false;
    chrBlock     = 0;
    mirrorChr    = false;
    for(auto& c : characterBank) c = 0;
    updateCHRBanks();

    mirroringReg      = 0;
    advancedNtControl = false;
    disableNtRam      = false;
    ntRamSelectBit    = 0;
    for(auto& r : ntLowRegs) r = 0;
    for(auto& r : ntHighRegs) r = 0;
    for(auto& c : ntBanks) c = 0;
    updateCIRAMBanks();

    irqEnabled        = false;
    irqSource         = IrqSource::CpuM2Rise;
    irqCountDirection = 0;
    irqFunkyMode      = false;
    irqFunkyModeReg   = 0;
    irqSmallPrescaler = false;
    irqPrescaler      = 0;
    irqCounter        = 0;
    irqXorReg         = 0;
    irqLine           = 0;
    characterAddress  = 0;

    multiplyValue1 = 0;
    multiplyValue2 = 0;
    regRamValue    = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(prgRegs);
    s(exPrgMode);
    s(prgMode);
    s(enablePrgAt6000);
    s(programAt6000Bank);
    s(programBanks);

    s(chrLowRegs);
    s(chrHighRegs);
    s(chrLatch);
    s(chrBlockMode);
    s(chrBlock);
    s(mirrorChr);
    s(characterBank);

    s(mirroringReg);
    s(advancedNtControl);
    s(disableNtRam);
    s(ntRamSelectBit);
    s(ntLowRegs);
    s(ntHighRegs);
    s(ntBanks);

    s(irqEnabled);
    s(irqSource);
    s(irqCountDirection);
    s(irqFunkyMode);
    s(irqFunkyModeReg);
    s(irqSmallPrescaler);
    s(irqPrescaler);
    s(irqCounter);
    s(irqXorReg);
    s(irqLine);
    s(characterAddress);

    s(multiplyValue1);
    s(multiplyValue2);
    s(regRamValue);
  }

  auto useAdvancedNt() const -> bool {
    // Mapper 211 always uses advanced NT control
    // Mapper 090 never uses it
    // Mapper 209 uses the advancedNtControl flag
    if(revision == Revision::JYTypeA) return false;
    if(revision == Revision::JYTypeB) return true;
    return advancedNtControl;
  }

  auto invertPrgBits(n8 reg) -> n8 {
    return (reg & 0x01) << 6 | (reg & 0x7e) >> 1;
  }

  auto tickIrqCounter() -> void {
    bool clockIrqCounter = false;
    n8   mask            = irqSmallPrescaler ? (n8)0x07 : (n8)0xff;
    n8   prescaler       = irqPrescaler & mask;

    if(irqCountDirection == 0x01) {  // Up
      if(prescaler == mask) {
        clockIrqCounter = true;
        prescaler       = 0;
      } else {
        prescaler++;
      }
    } else if(irqCountDirection == 0x02) {  // Down
      if(prescaler == 0) {
        clockIrqCounter = true;
        prescaler       = mask;
      } else {
        prescaler--;
      }
    }
    irqPrescaler = (irqPrescaler & ~mask) | (prescaler & mask);

    if(clockIrqCounter) {
      if(irqCountDirection == 0x01) {  // Up
        if(irqCounter == 0xff) {
          irqCounter = 0;
          if(irqEnabled) irqLine = 1;
        } else {
          irqCounter++;
        }
      } else if(irqCountDirection == 0x02) {  // Down
        if(irqCounter == 0) {
          irqCounter = 0xff;
          if(irqEnabled) irqLine = 1;
        } else {
          irqCounter--;
        }
      }
    }
  }

  // PRG registers
  n8 prgRegs[4];
  n3 prgMode;
  n2 exPrgMode;
  n1 enablePrgAt6000;
  n8 programAt6000Bank;  // bank at $6000-$7FFF when enabled
  n8 programBanks[4];    // 0. $8000-$9FFF
                         // 1. $A000-$BFFF
                         // 2. $C000-$DFFF
                         // 3. $E000-$FFFF

  // CHR registers
  n8  chrLowRegs[8];
  n8  chrHighRegs[8];
  n8  chrLatch[2];
  n2  chrMode;
  n1  chrBlockMode;
  n3  chrBlock;
  n1  mirrorChr;
  n16 characterBank[8];

  // Mirroring / Nametable
  n2 mirroringReg;
  n1 advancedNtControl;
  n1 disableNtRam;
  n8 ntRamSelectBit;
  n8 ntLowRegs[4];
  n8 ntHighRegs[4];
  n2 ntBanks[4];

  // IRQ
  n1        irqEnabled;
  IrqSource irqSource;
  n2        irqCountDirection;
  n1        irqFunkyMode;
  n8        irqFunkyModeReg;
  n1        irqSmallPrescaler;
  n8        irqPrescaler;
  n8        irqCounter;
  n8        irqXorReg;
  n1        irqLine;
  n16       characterAddress;

  // Misc
  n8 multiplyValue1;
  n8 multiplyValue2;
  n8 regRamValue;
};
