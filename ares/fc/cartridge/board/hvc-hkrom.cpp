struct HVC_HKROM : Interface {  //MMC6
  static auto create(string id) -> Interface* {
    if(id == "HVC-HKROM") return new HVC_HKROM;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

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
    tick();
  }

  auto irqTest(n32 address) -> void {
    if(!(characterAddress & 0x1000) && (address & 0x1000)) {
      if(irqDelay == 0) {
        if(irqCounter == 0) {
          irqCounter = irqLatch;
        } else if(--irqCounter == 0) {
          if(irqEnable) irqLine = 1;
        }
      }
      irqDelay = 6;
    }
    characterAddress = address;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x7000) return data;

    if(address < 0x8000) {
      if(!ramEnable) return data;
      if(!ramReadable[0] && !ramReadable[1]) return data;
      n1 bank = address >> 9 & 1;
      if(!ramReadable[bank]) return 0x00;
      return programRAM.read(bank << 9 | (n9)address);
    }

    n6 bank;
    switch(address >> 13 & 3) {
    case 0: bank = (programMode == 0 ? programBank[0] : (n6)0x3e); break;
    case 1: bank = programBank[1]; break;
    case 2: bank = (programMode == 1 ? programBank[0] : (n6)0x3e); break;
    case 3: bank = 0x3f; break;
    }
    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x7000) return;

    if(address < 0x8000) {
      if(!ramEnable) return;
      n1 bank = address >> 9 & 1;
      if(!ramWritable[bank]) return;
      return programRAM.write(bank << 9 | (n9)address, data);
    }

    switch(address & 0xe001) {
    case 0x8000:
      bankSelect = data.bit(0,2);
      ramEnable = data.bit(5);
      programMode = data.bit(6);
      characterMode = data.bit(7);
      if(!ramEnable) {
        ramReadable[0] = ramWritable[0] = 0;
        ramReadable[1] = ramWritable[1] = 0;
      }
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
      if(!ramEnable) break;
      ramWritable[0] = data.bit(4);
      ramReadable[0] = data.bit(5);
      ramWritable[1] = data.bit(6);
      ramReadable[1] = data.bit(7);
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
    return address >> mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    irqTest(address);
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(addressCHR(address));
    if(characterRAM) return characterRAM.read(addressCHR(address));
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    irqTest(address);
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(addressCHR(address), data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(characterMode);
    s(programMode);
    s(ramEnable);
    s(bankSelect);
    s(programBank);
    s(characterBank);
    s(mirror);
    s(ramReadable);
    s(ramWritable);
    s(irqLatch);
    s(irqCounter);
    s(irqEnable);
    s(irqDelay);
    s(irqLine);
    s(characterAddress);
  }

  n1  characterMode;
  n1  programMode;
  n1  ramEnable;
  n3  bankSelect;
  n6  programBank[2];
  n8  characterBank[6];
  n1  mirror;
  n1  ramReadable[2];
  n1  ramWritable[2];
  n8  irqLatch;
  n8  irqCounter;
  n1  irqEnable;
  n1  irqDelay;
  n1  irqLine;
  n16 characterAddress;
};
