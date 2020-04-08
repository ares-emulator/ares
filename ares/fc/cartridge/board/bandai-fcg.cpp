struct BandaiFCG : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;
  X24C01 eeprom;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
    if(auto memory = board["memory(type=EEPROM,content=Save)"]) {
      eeprom.erase();
      if(auto fp = platform->open(cartridge.node, "save.eeprom", File::Read)) {
        fp->read(eeprom.memory, min(128, fp->size()));
      }
    }
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
    if(auto memory = board["memory(type=EEPROM,content=Save)"]) {
      if(auto fp = platform->open(cartridge.node, "save.eeprom", File::Write)) {
        fp->write(eeprom.memory, 128);
      }
    }
  }

  auto main() -> void {
    if(irqCounterEnable) {
      if(--irqCounter == 0xffff) {
        cpu.irqLine(1);
        irqCounterEnable = false;
      }
    }
    tick();
  }

  auto addressCIRAM(uint address) const -> uint {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;
    case 2: return 0x0000 | address & 0x03ff;
    case 3: return 0x0400 | address & 0x03ff;
    }
    unreachable;
  }

  auto readPRG(uint address) -> uint8 {
    uint8 data = cpu.mdr();

    if(address >= 0x6000 && address <= 0x7fff) {
      data.bit(4) = eeprom.read();
      return data;
    }

    if(address & 0x8000) {
      uint1 region = bool(address & 0x4000);
      uint4 bank = (region == 0 ? programBank : (uint4)0x0f);
      return programROM.read(bank << 14 | (uint14)address);
    }

    return data;
  }

  auto writePRG(uint address, uint8 data) -> void {
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
        uint1 scl = data.bit(5);
        uint1 sda = data.bit(6);
        eeprom.write(scl, sda);
        break;
      }
    }
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    address = characterBank[address >> 10] << 10 | (uint10)address;
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return 0x00;
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    address = characterBank[address >> 10] << 10 | (uint10)address;
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void {
    reset();
  }

  auto reset() -> void {
    eeprom.reset();
    for(auto& bank : characterBank) bank = 0;
    programBank = 0;
    mirror = 0;
    irqCounterEnable = 0;
    irqCounter = 0;
    irqLatch = 0;
  }

  auto serialize(serializer& s) -> void {
    characterRAM.serialize(s);
    eeprom.serialize(s);
    s.array(characterBank);
    s.integer(programBank);
    s.integer(mirror);
    s.integer(irqCounterEnable);
    s.integer(irqCounter);
    s.integer(irqLatch);
  }

   uint8 characterBank[8];
   uint4 programBank;
   uint2 mirror;
   uint1 irqCounterEnable;
  uint16 irqCounter;
  uint16 irqLatch;
};
