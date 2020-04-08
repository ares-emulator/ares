struct KonamiVRC3 : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Writable<uint8> characterRAM;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
    mirror = board["mirror/mode"].string() == "vertical";
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto main() -> void {
    if(irqEnable) {
      if(irqMode == 0) {  //16-bit
        if(++irqCounter.bit(0,15) == 0) {
          irqLine = 1;
          irqEnable = irqAcknowledge;
          irqCounter.bit(0,15) = irqLatch;
        }
      }
      if(irqMode == 1) {  //8-bit
        if(++irqCounter.bit(0,7) == 0) {
          irqLine = 1;
          irqEnable = irqAcknowledge;
          irqCounter.bit(0,7) = irqLatch;
        }
      }
    }
    cpu.irqLine(irqLine);
    tick();
  }

  auto readPRG(uint address) -> uint8 {
    if(address < 0x6000) return cpu.mdr();
    if(address < 0x8000) return programRAM.read((uint13)address);

    uint4 bank = (address < 0xc000 ? programBank : (uint4)0xf);
    address = bank << 14 | (uint14)address;
    return programROM.read(address);
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write((uint13)address, data);

    switch(address & 0xf000) {
    case 0x8000: irqLatch.bit( 0, 3) = data.bit(0,3); break;
    case 0x9000: irqLatch.bit( 4, 7) = data.bit(0,3); break;
    case 0xa000: irqLatch.bit( 8,11) = data.bit(0,3); break;
    case 0xb000: irqLatch.bit(12,15) = data.bit(0,3); break;
    case 0xc000:
      irqAcknowledge = data.bit(0);
      irqEnable = data.bit(1);
      irqMode = data.bit(2);
      if(irqEnable) irqCounter = irqLatch;
      break;
    case 0xd000:
      irqLine = 0;
      irqEnable = irqAcknowledge;
      break;
    case 0xf000:
      programBank = data.bit(0,3);
      break;
    }
  }

  auto addressCIRAM(uint address) const -> uint {
    return address >> !mirror & 0x0400 | address & 0x03ff;
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterRAM) return characterRAM.read(address);
    return 0x00;
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void {
    programBank = 0;
    irqMode = 0;
    irqEnable = 0;
    irqAcknowledge = 0;
    irqLatch = 0;
    irqCounter = 0;
    irqLine = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    characterRAM.serialize(s);
    s.integer(mirror);
    s.integer(programBank);
    s.integer(irqMode);
    s.integer(irqEnable);
    s.integer(irqAcknowledge);
    s.integer(irqLatch);
    s.integer(irqCounter);
    s.integer(irqLine);
  }

   uint1 mirror;
   uint4 programBank;
   uint1 irqMode;
   uint1 irqEnable;
   uint1 irqAcknowledge;
  uint16 irqLatch;
  uint16 irqCounter;
   uint1 irqLine;
};
