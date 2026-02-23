struct KonamiVRC1 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "KONAMI-VRC-1") return new KonamiVRC1;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(characterRAM, "character.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;

    n4 bank;
    if((address & 0xe000) == 0x8000) bank = programBank[0];
    if((address & 0xe000) == 0xa000) bank = programBank[1];
    if((address & 0xe000) == 0xc000) bank = programBank[2];
    if((address & 0xe000) == 0xe000) bank = 0xf;
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;

    switch(address & 0xf000) {
    case 0x8000:
      programBank[0] = data.bit(0,3);
      break;
    case 0x9000:
      mirror = data.bit(0);
      characterBank[0].bit(4) = data.bit(1);
      characterBank[1].bit(4) = data.bit(2);
      break;
    case 0xa000:
      programBank[1] = data.bit(0,3);
      break;
    case 0xc000:
      programBank[2] = data.bit(0,3);
      break;
    case 0xe000:
      characterBank[0].bit(0,3) = data.bit(0,3);
      break;
    case 0xf000:
      characterBank[1].bit(0,3) = data.bit(0,3);
      break;
    }
  }

  auto addressCIRAM(n32 address) const -> n32 {
    return address >> mirror & 0x0400 | address & 0x03ff;
  }

  auto addressCHR(n32 address) const -> n32 {
    n1 bank = bool(address & 0x1000);
    return characterBank[bank] << 12 | (n12)address;
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
    s(characterRAM);
    s(programBank);
    s(characterBank);
    s(mirror);
  }

  n4 programBank[3];
  n5 characterBank[2];
  n1 mirror;
};
