struct BandaiLZ93D50 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "BANDAI-LZ93D50") return new BandaiLZ93D50;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;
  M24C eeprom;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    if(auto fp = pak->read("save.eeprom")) {
      if(fp->size() == 128) eeprom.load(M24C::Type::X24C01);
      if(fp->size() == 256) eeprom.load(M24C::Type::M24C02);
      if(eeprom) fp->read(eeprom.memory, eeprom.size());
    }
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
    if(auto fp = pak->write("save.eeprom")) {
      if(eeprom) fp->write(eeprom.memory, eeprom.size());
    }
  }

  auto main() -> void override {
    if(irqEnable) {
      if(--irqCounter == 0xffff) {
        cpu.irqLine(1);
        irqEnable = false;
      }
    }
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) {
      if(programRAM && ramEnable) return programRAM.read((n13)address);
      if(eeprom) data.bit(4) = eeprom.read();
      return data;
    }

    n5 bank = (address < 0xc000 ? (n5)programBank : (n5)0xf);
    if(characterRAM) bank.bit(4) = bool(programOuterBank);
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) {
      if(programRAM && ramEnable) return programRAM.write((n13)address, data);
    }

    switch(address & 0x800f) {
    case 0x8000: characterBank[0] = data; programOuterBank.bit(0) = data.bit(0); break;
    case 0x8001: characterBank[1] = data; programOuterBank.bit(1) = data.bit(0); break;
    case 0x8002: characterBank[2] = data; programOuterBank.bit(2) = data.bit(0); break;
    case 0x8003: characterBank[3] = data; programOuterBank.bit(3) = data.bit(0); break;
    case 0x8004: characterBank[4] = data; break;
    case 0x8005: characterBank[5] = data; break;
    case 0x8006: characterBank[6] = data; break;
    case 0x8007: characterBank[7] = data; break;
    case 0x8008: programBank = data.bit(0,3); break;
    case 0x8009: mirror = data.bit(0,1); break;
    case 0x800a:
      irqEnable = data.bit(0);
      irqCounter = irqLatch;
      cpu.irqLine(0);
      break;
    case 0x800b: irqLatch.byte(0) = data; break;
    case 0x800c: irqLatch.byte(1) = data; break;
    case 0x800d:
      if(eeprom) {
        eeprom.clock = data.bit(5);
        eeprom.data  = data.bit(6);
        eeprom.write();
      }
      ramEnable = data.bit(5);
      break;
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
    if(characterRAM) return characterRAM.read(address);
    address = characterBank[address >> 10] << 10 | (n10)address;
    return characterROM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
    eeprom.power();
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(eeprom);
    s(characterBank);
    s(programBank);
    s(programOuterBank);
    s(mirror);
    s(ramEnable);
    s(irqEnable);
    s(irqCounter);
    s(irqLatch);
  }

  n8  characterBank[8];
  n4  programBank;
  n4  programOuterBank;
  n2  mirror;
  n1  ramEnable;
  n1  irqEnable;
  n16 irqCounter;
  n16 irqLatch;
};
