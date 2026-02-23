struct Tengen80003x : Interface {  //RAMBO-1
  static auto create(string id) -> Interface* {
    if(id == "TENGEN-800032") return new Tengen80003x(Revision::TENGEN_800032);
    if(id == "TENGEN-800037") return new Tengen80003x(Revision::TENGEN_800037);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    TENGEN_800032,
    TENGEN_800037,
  } revision;

  Tengen80003x(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto save() -> void override {
  }

  auto main() -> void override {
    if(irqDelay) irqDelay--;
    if(irqLineDelay && --irqLineDelay == 0) irqLine = 1;
    cpu.irqLine(irqLine);

    if(++irqCycleCounter == 0) {
      if(irqCycleMode) irqTest(1);
    }

    tick();
  }

  auto irqTest(n8 delay) -> void {
    if(irqCounter == 0) {
      irqCounter = irqLatch + 1;
    }
    if(--irqCounter == 0) {
      if(irqEnable) irqLineDelay = delay;
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n5 bank;
    switch(address >> 13 & 3) {
    case 0: bank = programBank[programMode == 0 ? 0 : 2]; break;
    case 1: bank = programBank[1]; break;
    case 2: bank = programBank[programMode == 0 ? 2 : 0]; break;
    case 3: bank = 0x1f; break;
    }

    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;

    switch(address & 0xe001) {
    case 0x8000:
      bankSelect = data.bit(0,3);
      characterMode = data.bit(5);
      programMode = data.bit(6);
      characterInvert = data.bit(7);
      break;
    case 0x8001:
      switch(bankSelect) {
      case 0: characterBank[0] = data; break;
      case 1: characterBank[1] = data; break;
      case 2: characterBank[2] = data; break;
      case 3: characterBank[3] = data; break;
      case 4: characterBank[4] = data; break;
      case 5: characterBank[5] = data; break;
      case 6: programBank[0] = data; break;
      case 7: programBank[1] = data; break;
      case 8: characterBank[6] = data; break;
      case 9: characterBank[7] = data; break;
      case 15: programBank[2] = data; break;
      }
      break;
    case 0xa000:
      mirror = data.bit(0);
      break;
    case 0xc000:
      irqLatch = data;
      break;
    case 0xc001:
      irqCounter = 0;
      irqCycleCounter = 0;
      irqCycleMode = data.bit(0);
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

  auto addressCHR(n32 address) const -> n32 {
    address.bit(12) ^= characterInvert;
    if(characterMode == 0) {
      if(address <= 0x07ff) return (characterBank[0] & ~1) << 10 | (n11)address;
      if(address <= 0x0fff) return (characterBank[1] & ~1) << 10 | (n11)address;
    } else {
      if(address <= 0x03ff) return characterBank[0] << 10 | (n10)address;
      if(address <= 0x07ff) return characterBank[6] << 10 | (n10)address;
      if(address <= 0x0bff) return characterBank[1] << 10 | (n10)address;
      if(address <= 0x0fff) return characterBank[7] << 10 | (n10)address;
    }
    if(address <= 0x13ff) return characterBank[2] << 10 | (n10)address;
    if(address <= 0x17ff) return characterBank[3] << 10 | (n10)address;
    if(address <= 0x1bff) return characterBank[4] << 10 | (n10)address;
    if(address <= 0x1fff) return characterBank[5] << 10 | (n10)address;
    unreachable;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::TENGEN_800037) {
      address.bit(12) = characterInvert;
      if(characterMode == 0) {
        if(address <= 0x27ff) return characterBank[0].bit(7) << 10 | (n10)address;
        if(address <= 0x2fff) return characterBank[1].bit(7) << 10 | (n10)address;
      } else {
        if(address <= 0x23ff) return characterBank[0].bit(7) << 10 | (n10)address;
        if(address <= 0x27ff) return characterBank[6].bit(7) << 10 | (n10)address;
        if(address <= 0x2bff) return characterBank[1].bit(7) << 10 | (n10)address;
        if(address <= 0x2fff) return characterBank[7].bit(7) << 10 | (n10)address;
      }
      if(address <= 0x33ff) return characterBank[2].bit(7) << 10 | (n10)address;
      if(address <= 0x37ff) return characterBank[3].bit(7) << 10 | (n10)address;
      if(address <= 0x3bff) return characterBank[4].bit(7) << 10 | (n10)address;
      if(address <= 0x3fff) return characterBank[5].bit(7) << 10 | (n10)address;
      unreachable;
    }
    return address >> mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterROM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto power() -> void override {
  }

  auto ppuAddressBus(n14 address) -> void override {
    if(!(characterAddress & 0x1000) && (address & 0x1000)) {
      if(irqDelay == 0) {
        if(!irqCycleMode) irqTest(3);
      }
      irqDelay = 6;
    }
    characterAddress = address;
  }

  auto serialize(serializer& s) -> void override {
    s(characterMode);
    s(characterInvert);
    s(programMode);
    s(bankSelect);
    s(programBank);
    s(characterBank);
    s(mirror);
    s(irqLatch);
    s(irqCounter);
    s(irqEnable);
    s(irqDelay);
    s(irqLineDelay);
    s(irqLine);
    s(irqCycleMode);
    s(irqCycleCounter);
    s(characterAddress);
  }

  n1  characterMode;
  n1  characterInvert;
  n1  programMode;
  n4  bankSelect;
  n5  programBank[3];
  n8  characterBank[8];
  n1  mirror;
  n8  irqLatch;
  n8  irqCounter;
  n1  irqEnable;
  n8  irqDelay;
  n8  irqLineDelay;
  n1  irqLine;
  n1  irqCycleMode;
  n2  irqCycleCounter;
  n16 characterAddress;
};
