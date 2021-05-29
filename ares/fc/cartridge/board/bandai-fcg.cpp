struct BandaiFCG : Interface {
  static auto create(string id) -> Interface* {
    if(id == "BANDAI-FCG") return new BandaiFCG;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;
  M24C eeprom;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    if(auto fp = pak->read("save.eeprom")) {
      eeprom.load(M24C::Type::X24C01);
      fp->read({eeprom.memory, eeprom.size()});
    }
  }

  auto save() -> void override {
    Interface::save(characterRAM, "save.ram");
    if(auto fp = pak->write("save.eeprom")) {
      fp->write({eeprom.memory, eeprom.size()});
    }
  }

  auto main() -> void override {
    if(irqCounterEnable) {
      if(--irqCounter == 0xffff) {
        cpu.irqLine(1);
        irqCounterEnable = false;
      }
    }
    tick();
  }

  auto addressCIRAM(n32 address) const -> n32 {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;
    case 2: return 0x0000 | address & 0x03ff;
    case 3: return 0x0400 | address & 0x03ff;
    }
    unreachable;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address >= 0x6000 && address <= 0x7fff) {
      data.bit(4) = eeprom.read();
      return data;
    }

    if(address & 0x8000) {
      n1 region = bool(address & 0x4000);
      n4 bank = (region == 0 ? programBank : (n4)0x0f);
      return programROM.read(bank << 14 | (n14)address);
    }

    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x6000) {
      switch(address & 0xf) {
      case 0x0: characterBank[0] = data; break;
      case 0x1: characterBank[1] = data; break;
      case 0x2: characterBank[2] = data; break;
      case 0x3: characterBank[3] = data; break;
      case 0x4: characterBank[4] = data; break;
      case 0x5: characterBank[5] = data; break;
      case 0x6: characterBank[6] = data; break;
      case 0x7: characterBank[7] = data; break;
      case 0x8: programBank = data.bit(0,3); break;
      case 0x9: mirror = data.bit(0,1); break;
      case 0xa:
        cpu.irqLine(0);
        irqCounterEnable = data.bit(0);
        irqCounter = irqLatch;
        break;
      case 0xb: irqLatch.byte(0) = data; break;
      case 0xc: irqLatch.byte(1) = data; break;
      case 0xd:
        eeprom.clock = data.bit(5);
        eeprom.data  = data.bit(6);
        eeprom.write();
        break;
      }
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    address = characterBank[address >> 10] << 10 | (n10)address;
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    address = characterBank[address >> 10] << 10 | (n10)address;
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
    eeprom.power();
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(eeprom);
    s(characterBank);
    s(programBank);
    s(mirror);
    s(irqCounterEnable);
    s(irqCounter);
    s(irqLatch);
  }

  n8  characterBank[8];
  n4  programBank;
  n2  mirror;
  n1  irqCounterEnable;
  n16 irqCounter;
  n16 irqLatch;
};
