struct TaitoX1017 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "TAITO-X1-017") return new TaitoX1017;
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

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address >= 0x6000 && address < 0x7400) {
      if(!ramEnable[address >> 11 & 3]) return data;
      return programRAM.read((n13)address);
    }
    if(address < 0x8000) return data;

    n6 bank;
    switch(address >> 13 & 3) {
    case 0: bank = programBank[0]; break;
    case 1: bank = programBank[1]; break;
    case 2: bank = programBank[2]; break;
    case 3: bank = 0x3f; break;
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x6000 && address < 0x7400) {
      if(!ramEnable[address >> 11 & 3]) return;
      return programRAM.write((n13)address, data);
    }

    switch(address) {
    case 0x7ef0: characterBank[0] = data >> 1; break;
    case 0x7ef1: characterBank[1] = data >> 1; break;
    case 0x7ef2: characterBank[2] = data; break;
    case 0x7ef3: characterBank[3] = data; break;
    case 0x7ef4: characterBank[4] = data; break;
    case 0x7ef5: characterBank[5] = data; break;
    case 0x7ef6:
      mirror = data.bit(0);
      characterMode = data.bit(1);
      break;
    case 0x7ef7: ramEnable[0] = data == 0xca; break;
    case 0x7ef8: ramEnable[1] = data == 0x69; break;
    case 0x7ef9: ramEnable[2] = data == 0x84; break;
    case 0x7efa: programBank[0] = data >> 2; break;
    case 0x7efb: programBank[1] = data >> 2; break;
    case 0x7efc: programBank[2] = data >> 2; break;
    }
  }

  auto addressCHR(n32 address) const -> n32 {
    if(characterMode) address ^= 0x1000;
    if(address <= 0x07ff) return characterBank[0] << 11 | (n11)address;
    if(address <= 0x0fff) return characterBank[1] << 11 | (n11)address;
    if(address <= 0x13ff) return characterBank[2] << 10 | (n10)address;
    if(address <= 0x17ff) return characterBank[3] << 10 | (n10)address;
    if(address <= 0x1bff) return characterBank[4] << 10 | (n10)address;
    if(address <= 0x1fff) return characterBank[5] << 10 | (n10)address;
    unreachable;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    return address >> !mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    if(characterROM) return characterROM.read(addressCHR(address));
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(mirror);
    s(ramEnable);
    s(programBank);
    s(characterBank);
    s(characterMode);
  }

  n1 mirror;
  n1 ramEnable[3];
  n6 programBank[3];
  n8 characterBank[6];
  n1 characterMode;
};
