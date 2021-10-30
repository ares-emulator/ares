struct IremH3001 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "IREM-H3001") return new IremH3001;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto main() -> void override {
    if(irqEnable) {
      if(irqCounter && --irqCounter == 0) irqLine = 1;
    }
    cpu.irqLine(irqLine);
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n6 bank;
    if(programMode) {
      switch(address >> 13 & 3) {
      case 0: bank = 0x3e; break;
      case 1: bank = programBank[1]; break;
      case 2: bank = programBank[0]; break;
      case 3: bank = 0x3f; break;
      }
    } else {
      switch(address >> 13 & 3) {
      case 0: bank = programBank[0]; break;
      case 1: bank = programBank[1]; break;
      case 2: bank = 0x3e; break;
      case 3: bank = 0x3f; break;
      }
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;

    switch(address) {
    case 0x8000: programBank[0] = data.bit(0,5); break;
    case 0xa000: programBank[1] = data.bit(0,5); break;
    case 0xb000: characterBank[0] = data; break;
    case 0xb001: characterBank[1] = data; break;
    case 0xb002: characterBank[2] = data; break;
    case 0xb003: characterBank[3] = data; break;
    case 0xb004: characterBank[4] = data; break;
    case 0xb005: characterBank[5] = data; break;
    case 0xb006: characterBank[6] = data; break;
    case 0xb007: characterBank[7] = data; break;
    case 0x9000: programMode = data.bit(7); break;
    case 0x9001: mirror = data.bit(6,7); break;
    case 0x9003:
      irqEnable = data.bit(7);
      irqLine = 0;
      break;
    case 0x9004:
      irqCounter = irqLatch;
      irqLine = 0;
      break;
    case 0x9005: irqLatch = irqLatch.bit(0,7)  << 0 | data << 8; break;
    case 0x9006: irqLatch = irqLatch.bit(8,15) << 8 | data << 0; break;
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | (n10)address;
    case 1: return (n10)address;
    case 2: return address >> 1 & 0x0400 | (n10)address;
    case 3: return (n10)address;
    }
    unreachable;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) {
      n8 bank = characterBank[address >> 10 & 7];
      return characterROM.read(bank << 10 | (n10)address);
    }
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
    s(programMode);
    s(mirror);
    s(irqLatch);
    s(irqCounter);
    s(irqEnable);
    s(irqLine);
  }

  n6  programBank[2];
  n8  characterBank[8];
  n1  programMode;
  n2  mirror;
  n16 irqLatch;
  n16 irqCounter;
  n1  irqEnable;
  n1  irqLine;
};
