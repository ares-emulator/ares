struct HVC_FMR : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Writable<uint8> characterRAM;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    fds.present = 1;

    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto main() -> void override {
    fds.main();
    tick();
  }

  auto readPRG(uint address) -> uint8 {
    if(address >= 0x4020 && address <= 0x409f) return fds.read(address, cpu.mdr());
    if(address >= 0x6000 && address <= 0xdfff) return programRAM.read(address - 0x6000);
    if(address >= 0xe000 && address <= 0xffff) return programROM.read(address - 0xe000);
    return cpu.mdr();
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address == 0x4025) mirror = data.bit(3);
    if(address >= 0x4020 && address <= 0x409f) return fds.write(address, data);
    if(address >= 0x6000 && address <= 0xdfff) return programRAM.write(address - 0x6000, data);
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) {
      address = address >> mirror & 0x0400 | address & 0x03ff;
      return ppu.readCIRAM(address);
    }
    return characterRAM.read(address);
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) {
      address = address >> mirror & 0x0400 | address & 0x03ff;
      return ppu.writeCIRAM(address, data);
    }
    return characterRAM.write(address, data);
  }

  auto power() -> void {
    fds.power();
    mirror = 0;
  }

  auto serialize(serializer& s) -> void {
    fds.serialize(s);
    programRAM.serialize(s);
    characterRAM.serialize(s);
    s.integer(mirror);
  }

  uint1 mirror;
};
