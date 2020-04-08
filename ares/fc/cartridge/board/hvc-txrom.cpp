struct HVC_TxROM : Interface {  //MMC3
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;

  enum class Revision : uint {
    TBROM,
    TEROM,
    TFROM,
    TGROM,
    TKROM,
    TKSROM,
    TLROM,
    TL1ROM,
    TL2ROM,
    TLSROM,
    TNROM,
    TQROM,
    TR1ROM,
    TSROM,
    TVROM,
  } revision;

  HVC_TxROM(Markup::Node document, Revision revision) : Interface(document), revision(revision) {}

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto main() -> void override {
    if(irqDelay) irqDelay--;
    cpu.irqLine(irqLine);
    tick();
  }

  auto irqTest(uint address) -> void {
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

  auto readPRG(uint address) -> uint8 {
    if(address < 0x6000) return cpu.mdr();

    if(address < 0x8000) {
      if(!ramEnable) return cpu.mdr();
      return programRAM.read((uint13)address);
    }

    uint6 bank;
    switch(address >> 13 & 3) {
    case 0: bank = (programMode == 0 ? programBank[0] : (uint6)0x3e); break;
    case 1: bank = programBank[1]; break;
    case 2: bank = (programMode == 1 ? programBank[0] : (uint6)0x3e); break;
    case 3: bank = 0x3f; break;
    }
    address = bank << 13 | (uint13)address;
    return programROM.read(address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x6000) return;

    if(address < 0x8000) {
      if(!ramEnable || !ramWritable) return;
      return programRAM.write((uint13)address, data);
    }

    switch(address & 0xe001) {
    case 0x8000:
      bankSelect = data.bit(0,2);
      programMode = data.bit(6);
      characterMode = data.bit(7);
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
      ramWritable = !data.bit(6);
      ramEnable = data.bit(7);
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

  auto addressCHR(uint address) const -> uint {
    if(characterMode == 0) {
      if(address <= 0x07ff) return characterBank[0] << 10 | (uint11)address;
      if(address <= 0x0fff) return characterBank[1] << 10 | (uint11)address;
      if(address <= 0x13ff) return characterBank[2] << 10 | (uint10)address;
      if(address <= 0x17ff) return characterBank[3] << 10 | (uint10)address;
      if(address <= 0x1bff) return characterBank[4] << 10 | (uint10)address;
      if(address <= 0x1fff) return characterBank[5] << 10 | (uint10)address;
    } else {
      if(address <= 0x03ff) return characterBank[2] << 10 | (uint10)address;
      if(address <= 0x07ff) return characterBank[3] << 10 | (uint10)address;
      if(address <= 0x0bff) return characterBank[4] << 10 | (uint10)address;
      if(address <= 0x0fff) return characterBank[5] << 10 | (uint10)address;
      if(address <= 0x17ff) return characterBank[0] << 10 | (uint11)address;
      if(address <= 0x1fff) return characterBank[1] << 10 | (uint11)address;
    }
    unreachable;
  }

  auto addressCIRAM(uint address) const -> uint {
    return address >> mirror & 0x0400 | address & 0x03ff;
  }

  auto readCHR(uint address) -> uint8 {
    irqTest(address);
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(addressCHR(address));
    if(characterRAM) return characterRAM.read(addressCHR(address));
    return 0x00;
  }

  auto writeCHR(uint address, uint8 data) -> void {
    irqTest(address);
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(addressCHR(address), data);
  }

  auto power() -> void {
    characterMode = 0;
    programMode = 0;
    bankSelect = 0;
    for(auto& bank : programBank) bank = 0;
    for(auto& bank : characterBank) bank = 0;
    mirror = 0;
    ramEnable = 1;
    ramWritable = 1;
    irqLatch = 0;
    irqCounter = 0;
    irqEnable = 0;
    irqDelay = 0;
    irqLine = 0;
    characterAddress = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    characterRAM.serialize(s);
    s.integer(characterMode);
    s.integer(programMode);
    s.integer(bankSelect);
    s.array(programBank);
    s.array(characterBank);
    s.integer(mirror);
    s.integer(ramEnable);
    s.integer(ramWritable);
    s.integer(irqLatch);
    s.integer(irqCounter);
    s.integer(irqEnable);
    s.integer(irqDelay);
    s.integer(irqLine);
    s.integer(characterAddress);
  }

   uint1 characterMode;
   uint1 programMode;
   uint3 bankSelect;
   uint6 programBank[2];
   uint8 characterBank[6];
   uint1 mirror;
   uint1 ramEnable;
   uint1 ramWritable;
   uint8 irqLatch;
   uint8 irqCounter;
   uint1 irqEnable;
   uint8 irqDelay;
   uint1 irqLine;
  uint16 characterAddress;
};
