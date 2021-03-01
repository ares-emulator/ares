//SUNSOFT-4
//* After Burner (PRG-ROM + CHR-ROM)
//* Maharaja (PRG-ROM + PRG-RAM + CHR-ROM)
//* Nantettatte!! Baseball (PRG-ROM + EXT-ROM + CHR-ROM)
//note: EXT-ROM (option ROM) PCB ID is NTB-SUB-PCB and uses SUNSOFT-6 IC

struct Sunsoft4 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "SUNSOFT-4") return new Sunsoft4;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> optionROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(optionROM, "option.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
  }

  auto main() -> void override {
    if(optionTimer) optionTimer--;
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x6000) return data;

    if(address < 0x8000) {
      if(!ramEnable) return data;
      if(!programRAM) return data;
      return programRAM.read(address);
    }

    if(address < 0xc000 && optionROM && !optionDisable) {
      if(!optionTimer) return data;
      return optionROM.read((n14)address);
    }

    n3 bank;
    switch(address & 0xc000) {
    case 0x8000: bank = programBank; break;
    case 0xc000: bank = ~0; break;
    }
    return programROM.read(bank << 14 | (n14)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x6000) return;

    if(address < 0x8000) {
      if(!ramEnable) {
        optionTimer = 1024 * 105;
        return;
      }
      if(!programRAM) return;
      return programRAM.write(address, data);
    }

    switch(address & 0xf000) {
    case 0x8000: characterBank[0] = data; break;
    case 0x9000: characterBank[1] = data; break;
    case 0xa000: characterBank[2] = data; break;
    case 0xb000: characterBank[3] = data; break;
    case 0xc000: nametableBank[0] = data | 0x80; break;
    case 0xd000: nametableBank[1] = data | 0x80; break;
    case 0xe000: mirror = data.bit(0,1); nametableEnable = data.bit(4); break;
    case 0xf000: programBank = data.bit(0,2); optionDisable = data.bit(3); ramEnable = data.bit(4); break;
    }
  }

  auto addressCIRAM(n32 address) -> n32 {
    switch(mirror) {
    case 0: return address >> 0 & 0x0400 | address & 0x03ff;  //vertical
    case 1: return address >> 1 & 0x0400 | address & 0x03ff;  //horizontal
    case 2: return 0x0000 | address & 0x03ff;                 //first
    case 3: return 0x0400 | address & 0x03ff;                 //second
    }
    unreachable;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = addressCIRAM(address);
      if(nametableEnable) {
        n8 bank = nametableBank[address >> 10 & 1];
        return characterROM.read(bank << 10 | (n10)address);
      }
      return ppu.readCIRAM(address);
    }

    n8 bank;
    switch(address & 0x1800) {
    case 0x0000: bank = characterBank[0]; break;
    case 0x0800: bank = characterBank[1]; break;
    case 0x1000: bank = characterBank[2]; break;
    case 0x1800: bank = characterBank[3]; break;
    }
    return characterROM.read(bank << 11 | (n11)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = addressCIRAM(address);
      if(nametableEnable) return;
      return ppu.writeCIRAM(address, data);
    }
  }

  auto power() -> void override {
    nametableBank[0] = 0x80;
    nametableBank[1] = 0x81;
    optionDisable = 1;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(optionTimer);
    s(characterBank);
    s(nametableBank);
    s(programBank);
    s(optionDisable);
    s(ramEnable);
    s(nametableEnable);
    s(mirror);
  }

  n32 optionTimer;
  n8  characterBank[4];
  n8  nametableBank[2];
  n4  programBank;
  n1  optionDisable;
  n1  ramEnable;
  n1  nametableEnable;
  n2  mirror;
};
