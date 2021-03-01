//JALECO-JF-(23A,24A,25,27B,29A,37,40)
//todo: uPD7756 ADPCM unsupported

struct JalecoJF : Interface {
  static auto create(string id) -> Interface* {
    if(id == "JALECO-JF") return new JalecoJF;
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
    if(irqEnable) {
      if(!irqCounter--) {
        irqLine = 1;
        switch(irqMode) {
        case 0: irqCounter = n16(irqReload); break;
        case 1: irqCounter = n12(irqReload); break;
        case 2: irqCounter = n8 (irqReload); break;
        case 3: irqCounter = n4 (irqReload); break;
        }
      }
    }
    cpu.irqLine(irqLine);
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;

    if(address < 0x8000) {
      if(!programRAM) return data;
      return programRAM.read((n13)address);
    }

    n6 bank, banks = programROM.size() >> 13;
    switch(address & 0xe000) {
    case 0x8000: bank = programBank[0]; break;
    case 0xa000: bank = programBank[1]; break;
    case 0xc000: bank = programBank[2]; break;
    case 0xe000: bank = banks - 1; break;
    }
    address = bank << 13 | (n13)address;
    return programROM.read(address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;

    if(address < 0x8000) {
      if(!programRAM) return;
      return programRAM.write((n13)address, data);
    }

    switch(address & 0xf003) {
    case 0x8000: programBank[0].bit(0,3) = data.bit(0,3); break;
    case 0x8001: programBank[0].bit(4,5) = data.bit(0,1); break;
    case 0x8002: programBank[1].bit(0,3) = data.bit(0,3); break;
    case 0x8003: programBank[1].bit(4,5) = data.bit(0,1); break;
    case 0x9000: programBank[2].bit(0,3) = data.bit(0,3); break;
    case 0x9001: programBank[2].bit(4,5) = data.bit(0,1); break;
    case 0xa000: characterBank[0].bit(0,3) = data.bit(0,3); break;
    case 0xa001: characterBank[0].bit(4,7) = data.bit(0,3); break;
    case 0xa002: characterBank[1].bit(0,3) = data.bit(0,3); break;
    case 0xa003: characterBank[1].bit(4,7) = data.bit(0,3); break;
    case 0xb000: characterBank[2].bit(0,3) = data.bit(0,3); break;
    case 0xb001: characterBank[2].bit(4,7) = data.bit(0,3); break;
    case 0xb002: characterBank[3].bit(0,3) = data.bit(0,3); break;
    case 0xb003: characterBank[3].bit(4,7) = data.bit(0,3); break;
    case 0xc000: characterBank[4].bit(0,3) = data.bit(0,3); break;
    case 0xc001: characterBank[4].bit(4,7) = data.bit(0,3); break;
    case 0xc002: characterBank[5].bit(0,3) = data.bit(0,3); break;
    case 0xc003: characterBank[5].bit(4,7) = data.bit(0,3); break;
    case 0xd000: characterBank[6].bit(0,3) = data.bit(0,3); break;
    case 0xd001: characterBank[6].bit(4,7) = data.bit(0,3); break;
    case 0xd002: characterBank[7].bit(0,3) = data.bit(0,3); break;
    case 0xd003: characterBank[7].bit(4,7) = data.bit(0,3); break;
    case 0xe000: irqReload.bit( 0, 3) = data.bit(0,3); break;
    case 0xe001: irqReload.bit( 4, 7) = data.bit(0,3); break;
    case 0xe002: irqReload.bit( 8,11) = data.bit(0,3); break;
    case 0xe003: irqReload.bit(12,15) = data.bit(0,3); break;
    case 0xf000: irqCounter = irqReload; irqLine = 0; break;
    case 0xf001: irqEnable = data.bit(0); irqMode = data.bit(1,3); irqLine = 0; break;
    case 0xf002: mirror = data.bit(0,1); break;
    case 0xf003: break;  //uPD7756 ADPCM
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    switch(mirror) {
    case 0: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal mirroring
    case 1: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical mirroring
    case 2: return 0x0000 | address & 0x03ff;                 //one-screen mirroring (first)
    case 3: return 0x0400 | address & 0x03ff;                 //one-screen mirroring (second)
    }
    unreachable;
  }

  auto addressCHR(n32 address) const -> n32 {
    n8 bank = characterBank[address >> 10];
    return bank << 10 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterROM.read(addressCHR(address));
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto power() -> void override {
    for(u32 n : range(3)) programBank[n] = n;
    for(u32 n : range(8)) characterBank[n] = n;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(programBank);
    s(characterBank);
    s(irqLine);
    s(irqCounter);
    s(irqReload);
    s(irqEnable);
    s(irqMode);
    s(mirror);
  }

  n6  programBank[3];
  n8  characterBank[8];
  n1  irqLine;
  n16 irqCounter;
  n16 irqReload;
  n1  irqEnable;
  n3  irqMode;
  n2  mirror;
};
