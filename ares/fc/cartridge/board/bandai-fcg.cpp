struct BandaiFCG : Interface {
  static auto create(string id) -> Interface* {
    if(id == "BANDAI-FCG") return new BandaiFCG;
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
      if(--irqCounter == 0xffff) {
        cpu.irqLine(1);
        irqEnable = false;
      }
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    n4 bank = (address < 0xc000 ? programBank : (n4)0x0f);
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    switch(address & 0xe00f) {
    case 0x6000: characterBank[0] = data; break;
    case 0x6001: characterBank[1] = data; break;
    case 0x6002: characterBank[2] = data; break;
    case 0x6003: characterBank[3] = data; break;
    case 0x6004: characterBank[4] = data; break;
    case 0x6005: characterBank[5] = data; break;
    case 0x6006: characterBank[6] = data; break;
    case 0x6007: characterBank[7] = data; break;
    case 0x6008: programBank = data.bit(0,3); break;
    case 0x6009: mirror = data.bit(0,1); break;
    case 0x600a:
      irqEnable = data.bit(0);
      cpu.irqLine(0);
      break;
    case 0x600b: irqCounter.byte(0) = data; break;
    case 0x600c: irqCounter.byte(1) = data; break;
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | (n10)address;
    case 1: return address >> 1 & 0x0400 | (n10)address;
    case 2: return 0x0000 | (n10)address;
    case 3: return 0x0400 | (n10)address;
    }
    unreachable;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    address = characterBank[address >> 10] << 10 | (n10)address;
    return characterROM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(characterBank);
    s(programBank);
    s(mirror);
    s(irqEnable);
    s(irqCounter);
  }

  n8  characterBank[8];
  n4  programBank;
  n2  mirror;
  n1  irqEnable;
  n16 irqCounter;
};
