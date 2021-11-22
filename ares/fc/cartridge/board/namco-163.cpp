//todo: sound

struct Namco163 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "NAMCO-163") return new Namco163;
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
    cpu.irqLine(irqLine);
    if(irqEnable) {
      if(irqCounter != 0x7fff && ++irqCounter == 0x7fff) irqLine = 1;
    }
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x4800) return data;
    if(address < 0x5000) return data;
    //if(address < 0x5000) return soundRAM.read(soundAddress);

    if(address < 0x6000) {
      if(address < 0x5800) return irqCounter.bit(0,7);
      if(address < 0x6000) return irqCounter.bit(8,14) | irqEnable << 7;
    }

    if(address < 0x8000) {
      if(!programRAM) return data;
      return programRAM.read((n13)address);
    }

    n6 bank;
    switch(address >> 13 & 3) {
    case 0: bank = programBank[0]; break;
    case 1: bank = programBank[1]; break;
    case 2: bank = programBank[2]; break;
    case 3: bank = 0x3f; break;
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x6000 && address < 0x8000) {
      if(!programRAM) return;
      if(!ramEnable[address >> 11 & 3]) return;
      return programRAM.write((n13)address, data);
    }

    switch(address & 0xf800) {
    case 0x4800:
      //soundRAM.write(soundAddres, data);
      break;
    case 0x5000:
      irqCounter.bit(0,7) = data;
      irqLine = 0;
      break;
    case 0x5800:
      irqCounter.bit(8,14) = data.bit(0,6);
      irqEnable = data.bit(7);
      irqLine = 0;
      break;
    case 0x8000: characterBank[0]  = data; break;
    case 0x8800: characterBank[1]  = data; break;
    case 0x9000: characterBank[2]  = data; break;
    case 0x9800: characterBank[3]  = data; break;
    case 0xa000: characterBank[4]  = data; break;
    case 0xa800: characterBank[5]  = data; break;
    case 0xb000: characterBank[6]  = data; break;
    case 0xb800: characterBank[7]  = data; break;
    case 0xc000: characterBank[8]  = data; break;
    case 0xc800: characterBank[9]  = data; break;
    case 0xd000: characterBank[10] = data; break;
    case 0xd800: characterBank[11] = data; break;
    case 0xe000:
      programBank[0] = data.bit(0,5);
      //soundEnable = !data.bit(6);
      break;
    case 0xe800:
      programBank[1] = data.bit(0,5);
      ciramEnable[0] = !data.bit(6);
      ciramEnable[1] = !data.bit(7);
      break;
    case 0xf000:
      programBank[2] = data.bit(0,5);
      break;
    case 0xf800:
      ramEnable[0] = data.bit(4,7) == 4 && !data.bit(0);
      ramEnable[1] = data.bit(4,7) == 4 && !data.bit(1);
      ramEnable[2] = data.bit(4,7) == 4 && !data.bit(2);
      ramEnable[3] = data.bit(4,7) == 4 && !data.bit(3);
      //soundAddress = data.bit(0,6);
      //soundRepeat = data.bit(7);
      break;
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) address.bit(12) = 0;
    bool ciramSelect = address & 0x2000 || ciramEnable[address >> 12 & 1];
    n8 bank = characterBank[address >> 10];
    if(bank >= 0xe0 && ciramSelect) {
      return ppu.readCIRAM(bank.bit(0) << 10 | (n10)address);
    }
    return characterROM.read(bank << 10 | (n10)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) address.bit(12) = 0;
    bool ciramSelect = address & 0x2000 || ciramEnable[address >> 12 & 1];
    n8 bank = characterBank[address >> 10];
    if(bank >= 0xe0 && ciramSelect) {
      return ppu.writeCIRAM(bank.bit(0) << 10 | (n10)address, data);
    }
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(ramEnable);
    s(ciramEnable);
    s(programBank);
    s(characterBank);
    s(irqCounter);
    s(irqEnable);
    s(irqLine);
  }

  n1  ramEnable[4];
  n1  ciramEnable[2];
  n6  programBank[3];
  n8  characterBank[12];
  n15 irqCounter;
  n1  irqEnable;
  n1  irqLine;
};
