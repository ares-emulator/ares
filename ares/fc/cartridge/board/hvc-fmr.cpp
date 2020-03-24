//HVC-FMR-03
//HVC-FMR-04

struct HVC_FMR : Board {
  HVC_FMR(Markup::Node& document) : Board(document) {
    fds.present = 1;
  }

  auto main() -> void override {
    fds.main();
    tick();
  }

  auto readPRG(uint address) -> uint8 {
    if(address >= 0x4020 && address <= 0x409f) return fds.read(address, cpu.mdr());
    if(address >= 0x6000 && address <= 0xdfff) return prgram.read(address - 0x6000);
    if(address >= 0xe000 && address <= 0xffff) return prgrom.read(address);
    return cpu.mdr();
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address == 0x4025) mirror = data.bit(3);
    if(address >= 0x4020 && address <= 0x409f) return fds.write(address, data);
    if(address >= 0x6000 && address <= 0xdfff) return prgram.write(address - 0x6000, data);
  }

  auto readCHR(uint address) -> uint8 {
    if(address & 0x2000) {
      if(mirror == 1) address = ((address & 0x0800) >> 1) | (address & 0x03ff);
      return ppu.readCIRAM(address & 0x07ff);
    }
    return chrram.read(address);
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) {
      if(mirror == 1) address = ((address & 0x0800) >> 1) | (address & 0x03ff);
      return ppu.writeCIRAM(address & 0x07ff, data);
    }
    return chrram.write(address, data);
  }

  auto power() -> void {
    fds.power();
    mirror = 0;
  }

  auto serialize(serializer& s) -> void {
    Board::serialize(s);
    fds.serialize(s);
    s.integer(mirror);
  }

  uint1 mirror;
};
