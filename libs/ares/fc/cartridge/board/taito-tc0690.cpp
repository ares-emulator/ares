struct TaitoTC0690 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "TAITO-TC0690") return new TaitoTC0690;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
  }

  auto main() -> void override {
    if(irqDelay) irqDelay--;
    if(irqLineDelay && --irqLineDelay == 0) irqLine = 1;
    cpu.irqLine(irqLine);
    tick();
  }

  auto irqTest(n16 address) -> void {
    if(!(characterAddress & 0x1000) && (address & 0x1000)) {
      if(irqDelay == 0) {
        if(irqCounter == 0) {
          irqCounter = irqLatch + 1;
        }
        if(--irqCounter == 0) {
          if(irqEnable) irqLineDelay = 6;
        }
      }
      irqDelay = 6;
    }
    characterAddress = address;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n6 bank;
    switch(address >> 13 & 3) {
    case 0: bank = programBank[0]; break;
    case 1: bank = programBank[1]; break;
    case 2: bank = 0x3e; break;
    case 3: bank = 0x3f; break;
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;

    switch(address & 0xe003) {
    case 0x8000: case 0x8001:
      programBank[address & 1] = data.bit(0,5);
      break;
    case 0x8002: characterBank[0] = data; break;
    case 0x8003: characterBank[1] = data; break;
    case 0xa000: characterBank[2] = data; break;
    case 0xa001: characterBank[3] = data; break;
    case 0xa002: characterBank[4] = data; break;
    case 0xa003: characterBank[5] = data; break;
    case 0xc000:
      irqLatch = ~data;
      break;
    case 0xc001:
      irqCounter = 0;
      break;
    case 0xc002:
      irqEnable = 1;
      break;
    case 0xc003:
      irqEnable = 0;
      irqLine = 0;
      break;
    case 0xe000:
      mirror = data.bit(6);
      break;
    }
  }

  auto addressCHR(n32 address) const -> n32 {
    if(address <= 0x07ff) return characterBank[0] << 11 | (n11)address;
    if(address <= 0x0fff) return characterBank[1] << 11 | (n11)address;
    if(address <= 0x13ff) return characterBank[2] << 10 | (n10)address;
    if(address <= 0x17ff) return characterBank[3] << 10 | (n10)address;
    if(address <= 0x1bff) return characterBank[4] << 10 | (n10)address;
    if(address <= 0x1fff) return characterBank[5] << 10 | (n10)address;
    unreachable;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    return address >> mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    irqTest(address);
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterROM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    irqTest(address);
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(mirror);
    s(programBank);
    s(characterBank);
    s(irqLatch);
    s(irqCounter);
    s(irqEnable);
    s(irqDelay);
    s(irqLineDelay);
    s(irqLine);
    s(characterAddress);
  }

  n1  mirror;
  n6  programBank[2];
  n8  characterBank[6];
  n8  irqLatch;
  n8  irqCounter;
  n1  irqEnable;
  n8  irqDelay;
  n8  irqLineDelay;
  n1  irqLine;
  n16 characterAddress;
};
