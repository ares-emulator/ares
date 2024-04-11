struct Sunsoft3 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "SUNSOFT-3") return new Sunsoft3;
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
    if(irqEnable && !irqCounter--) {
      irqEnable = 0;
      cpu.irqLine(1);
    }
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;

    if(address < 0x8000) {
      if(!programRAM) return data;
      return programRAM.read(address);
    }

    n4 bank;
    switch(address & 0xc000) {
    case 0x8000: bank = programBank; break;
    case 0xc000: bank = ~0; break;
    }
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;

    if(address < 0x8000) {
      if(!programRAM) return;
      return programRAM.write(address, data);
    }

    if((address & 0x8800) == 0x8000) {
      cpu.irqLine(0);
      return;
    }

    switch(address & 0xf800) {
    case 0x8800: characterBank[0] = data.bit(0,5); break;
    case 0x9800: characterBank[1] = data.bit(0,5); break;
    case 0xa800: characterBank[2] = data.bit(0,5); break;
    case 0xb800: characterBank[3] = data.bit(0,5); break;
    case 0xc800:
      if(irqToggle == 0) irqCounter.bit(8,15) = data;
      if(irqToggle == 1) irqCounter.bit(0, 7) = data;
      irqToggle = !irqToggle;
      break;
    case 0xd800: irqEnable = data.bit(4); irqToggle = 0; break;
    case 0xe800: mirror = data.bit(0,1); break;
    case 0xf800: programBank = data.bit(0,4); break;
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal
    case 2: return 0x0000 | address & 0x03ff;                 //first
    case 3: return 0x0400 | address & 0x03ff;                 //second
    }
    unreachable;
  }

  auto addressCHR(n32 address) const -> n32 {
    n6 bank = characterBank[address >> 11 & 3];
    return bank << 11 | (n11)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(addressCHR(address));
    if(characterRAM) return characterRAM.read(addressCHR(address));
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
    if(characterRAM) return characterRAM.write(addressCHR(address), data);
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(programBank);
    s(characterBank);
    s(mirror);
    s(irqEnable);
    s(irqToggle);
    s(irqCounter);
  }

  n5  programBank;
  n6  characterBank[4];
  n2  mirror;
  n1  irqEnable;
  n1  irqToggle;
  n16 irqCounter;
};
