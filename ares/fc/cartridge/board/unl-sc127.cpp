struct Sc127 : Interface {
  static auto create(string id) -> Interface* {
    // Wario Land II (Kirby hack)
    if(id == "UNL-SC-127") return new Sc127;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
  }

  auto main() -> void override {
    if(irqDelay) irqDelay--;
    cpu.irqLine(irqLine);
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) {
      if(!programRAM) return data;
      return programRAM.read((n13)address);
    }

    n8 bank = programBank[(address >> 13) & 3];
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) {
      if(!programRAM) return;
      return programRAM.write((n13)address, data);
    }

    switch(address & 0xf007) {
    case 0x8000:
    case 0x8001:
    case 0x8002:
    case 0x8003: programBank[address & 3] = data; break;

    case 0x9000:
    case 0x9001:
    case 0x9002:
    case 0x9003:
    case 0x9004:
    case 0x9005:
    case 0x9006:
    case 0x9007: characterBank[address & 7] = data; break;

    case 0xc002:
      irqEnable = 0;
      irqLine   = 0;
      break;

    case 0xc003: irqEnable = 1; break;

    case 0xc005: irqCounter = data; break;

    case 0xd001: mirror = data.bit(0); break;
    }
  }

  auto addressCHR(n32 address) const -> n32 {
    return characterBank[address >> 10 & 7] << 10 | (n10)address;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    return address >> mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterROM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  //A12 rising edge detection with delay filtering (Mesen A12Watcher style)
  auto ppuAddressBus(n14 address) -> void override {
    if(!(characterAddress & 0x1000) && (address & 0x1000)) {
      if(irqDelay == 0) {
        if(irqEnable) {
          irqCounter--;
          if(irqCounter == 0) {
            irqEnable = 0;
            irqLine   = 1;
          }
        }
      }
      irqDelay = 6;
    }
    characterAddress = address;
  }

  auto power() -> void override {
    programBank[3]   = 0xff;
    characterBank[0] = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(programBank);
    s(characterBank);
    s(mirror);
    s(irqCounter);
    s(irqEnable);
    s(irqDelay);
    s(irqLine);
    s(characterAddress);
  }

  n8  programBank[4];
  n8  characterBank[8];
  n1  mirror;  // 1: horizontal, 0: vertical
  n8  irqCounter;
  n1  irqEnable;
  n8  irqDelay;
  n1  irqLine;
  n16 characterAddress;
};
