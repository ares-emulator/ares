struct SystemE : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n8 bank;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address < 0x8000) return rom.read(address);
    if(address < 0xc000) return rom.read(0x10000 + ((u32)(bank & 15) << 14) + (address & 0x3fff));
    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address < 0x8000 || address >= 0xc000) return;
    auto select = bank >> 5;
    auto& target = (select & 1) ? vdp2 : vdp;
    auto offset = (select & 1) ? ((select & 4) ? 0 : 0x4000) : ((select & 2) ? 0 : 0x4000);
    target.vram[offset | (address & 0x3fff)] = data;
  }

  auto out(n16 address, n8 data) -> void override {
    if((address & 0xff) != 0xf7) return;
    bank = data;
    vdp2.vramBank = data.bit(7);
    vdp.vramBank = data.bit(6);
  }

  auto power() -> void override { bank = 0; }
  auto serialize(serializer& s) -> void override { s(bank); }
};
