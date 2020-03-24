//BANDAI-FCG

struct BandaiFCG : Board {
  BandaiFCG(Markup::Node& document) : Board(document) {
  }

  auto load() -> void override {
    Board::load();
    auto manifest = BML::unserialize(cartridge.manifest());

    if(auto memory = manifest["game/board/memory(type=EEPROM,content=Save)"]) {
      eeprom.erase();
      if(auto fp = platform->open(cartridge.node, "save.eeprom", File::Read)) {
        fp->read(eeprom.memory, min(128, fp->size()));
      }
    }
  }

  auto save() -> void override {
    Board::save();
    auto manifest = BML::unserialize(cartridge.manifest());

    if(auto memory = manifest["game/board/memory(type=EEPROM,content=Save)"]) {
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

  auto addrCIRAM(uint addr) const -> uint {
    switch(mirror) {
    case 0: return (addr & 0x0400) >> 0 | addr & 0x03ff;
    case 1: return (addr & 0x0800) >> 1 | addr & 0x03ff;
    case 2: return 0x0000 | addr & 0x03ff;
    case 3: return 0x0400 | addr & 0x03ff;
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
      bool region = address & 0x4000;
      uint bank = (region == 0 ? prgBank : (uint8)0x0f);
      return prgrom.read(bank << 14 | address & 0x3fff);
    }

    return data;
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address >= 0x6000) {
      switch(address & 15) {
      case 0x00: case 0x01: case 0x02: case 0x03:
      case 0x04: case 0x05: case 0x06: case 0x07:
        chrBank[address & 7] = data;
        break;
      case 0x08:
        prgBank = data & 0x0f;
        break;
      case 0x09:
        mirror = data & 0x03;
        break;
      case 0x0a:
        cpu.irqLine(0);
        irqCounterEnable = data & 0x01;
        irqCounter = irqLatch;
        break;
      case 0x0b:
        irqLatch = irqLatch & 0xff00 | data << 0;
        break;
      case 0x0c:
        irqLatch = irqLatch & 0x00ff | data << 8;
        break;
      case 0x0d:
        uint1 scl = data.bit(5);
        uint1 sda = data.bit(6);
        eeprom.write(scl, sda);
        break;
      }
    }
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) return ppu.readCIRAM(addrCIRAM(address));
    address = chrBank[address >> 10] << 10 | address & 0x03ff;
    return Board::readCHR(address);
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) return ppu.writeCIRAM(addrCIRAM(address), data);
    address = chrBank[address >> 10] << 10 | address & 0x03ff;
    return Board::writeCHR(address, data);
  }

  auto power() -> void {
    reset();
  }

  auto reset() -> void {
    eeprom.reset();
    for(auto& data : chrBank) data = 0;
    prgBank = 0;
    mirror = 0;
    irqCounterEnable = 0;
    irqCounter = 0;
    irqLatch = 0;
  }

  auto serialize(serializer& s) -> void {
    Board::serialize(s);
    eeprom.serialize(s);

    s.array(chrBank);
    s.integer(prgBank);
    s.integer(mirror);
    s.integer(irqCounterEnable);
    s.integer(irqCounter);
    s.integer(irqLatch);
  }

  X24C01 eeprom;
   uint8 chrBank[8];
   uint8 prgBank;
   uint2 mirror;
   uint1 irqCounterEnable;
  uint16 irqCounter;
  uint16 irqLatch;
};
