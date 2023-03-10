struct TaitoX1005 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "TAITO-X1-005" ) return new TaitoX1005(Revision::X1005);
    if(id == "TAITO-X1-005A") return new TaitoX1005(Revision::X1005A);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;

  enum class Revision : u32 {
    X1005,
    X1005A,
  } revision;

  TaitoX1005(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x7f00) return data;

    if(address < 0x8000) {
      if(!ramEnable) return data;
      return programRAM.read((n7)address);
    }

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
    if(address >= 0x7f00 && address < 0x8000) {
      if(!ramEnable) return;
      return programRAM.write((n7)address, data);
    }

    switch(address) {
    case range2(0x7ef0, 0x7ef1):
      characterBank[address & 1] = data >> 1;
      nametableBank[address & 1] = data.bit(7);
      break;
    case 0x7ef2: characterBank[2] = data; break;
    case 0x7ef3: characterBank[3] = data; break;
    case 0x7ef4: characterBank[4] = data; break;
    case 0x7ef5: characterBank[5] = data; break;
    case range2(0x7ef6, 0x7ef7): mirror = data.bit(0); break;
    case range2(0x7ef8, 0x7ef9): ramEnable = data == 0xa3; break;
    case range2(0x7efa, 0x7efb): programBank[0] = data.bit(0,5); break;
    case range2(0x7efc, 0x7efd): programBank[1] = data.bit(0,5); break;
    case range2(0x7efe, 0x7eff): programBank[2] = data.bit(0,5); break;
    }
  }

  auto addressCHR(n32 address) const -> n32 {
    if(address <= 0x07ff) return characterBank[0] << 11 | (n11)address;
    if(address <= 0x0fff) return characterBank[1] << 11 | (n11)address;
    if(address <= 0x13ff) return characterBank[2] << 10 | (n10)address;
    if(address <= 0x17ff) return characterBank[3] << 10 | (n10)address;
    if(address <= 0x1bff) return characterBank[4] << 10 | (n10)address;
    if(address <= 0x1fff) return characterBank[5] << 10 | (n10)address;
    unreachable;
  }

  auto addressCIRAM(n32 address) const -> n32 {
    if(revision == Revision::X1005A) {
      return nametableBank[address >> 12 & 1] << 10 | (n10)address;
    }
    return address >> !mirror & 0x0400 | (n10)address;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterROM.read(addressCHR(address));
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
    s(nametableBank);
  }

  n1 mirror;
  n1 ramEnable;
  n6 programBank[3];
  n8 characterBank[6];
  n1 nametableBank[2];
};
