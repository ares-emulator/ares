struct KonamiVRC3 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "KONAMI-VRC-3") return new KonamiVRC3;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterRAM, "character.ram");
    mirror = pak->attribute("mirror") == "vertical";
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
  }

  auto main() -> void override {
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

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;
    if(address < 0x8000) return programRAM.read((n13)address);

    n4 bank = (address < 0xc000 ? programBank : (n4)0xf);
    address = bank << 14 | (n14)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;
    if(address < 0x8000) return programRAM.write((n13)address, data);

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

  auto addressCIRAM(n32 address) const -> n32 {
    return address >> !mirror & 0x0400 | address & 0x03ff;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterRAM) return characterRAM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(mirror);
    s(programBank);
    s(irqMode);
    s(irqEnable);
    s(irqAcknowledge);
    s(irqLatch);
    s(irqCounter);
    s(irqLine);
  }

  n1  mirror;
  n4  programBank;
  n1  irqMode;
  n1  irqEnable;
  n1  irqAcknowledge;
  n16 irqLatch;
  n16 irqCounter;
  n1  irqLine;
};
