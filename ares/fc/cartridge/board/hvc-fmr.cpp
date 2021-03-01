struct HVC_FMR : Interface {
  static auto create(string id) -> Interface* {
    if(id == "HVC-FMR") return new HVC_FMR;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Writable<n8> characterRAM;

  auto load() -> void override {
    fds.present = 1;
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterRAM, "character.ram");
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
  }

  auto main() -> void override {
    fds.main();
    tick();
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address >= 0x4020 && address <= 0x409f) return fds.read(address, data);
    if(address >= 0x6000 && address <= 0xdfff) return programRAM.read(address - 0x6000);
    if(address >= 0xe000 && address <= 0xffff) return programROM.read(address - 0xe000);
    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address == 0x4025) mirror = data.bit(3);
    if(address >= 0x4020 && address <= 0x409f) return fds.write(address, data);
    if(address >= 0x6000 && address <= 0xdfff) return programRAM.write(address - 0x6000, data);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) {
      address = address >> mirror & 0x0400 | (n10)address;
      return ppu.readCIRAM(address);
    }
    return characterRAM.read(address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      address = address >> mirror & 0x0400 | (n10)address;
      return ppu.writeCIRAM(address, data);
    }
    return characterRAM.write(address, data);
  }

  auto power() -> void override {
    fds.power();
  }

  auto serialize(serializer& s) -> void override {
    s(fds);
    s(programRAM);
    s(characterRAM);
    s(mirror);
  }

  n1 mirror;
};
