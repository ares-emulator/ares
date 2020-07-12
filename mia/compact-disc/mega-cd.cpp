struct MegaCD : CompactDisc {
  auto name() -> string override { return "Mega CD"; }
  auto manifestSector() const -> uint override { return 0; }
  auto manifest(vector<uint8_t> sector, string location) -> string override;
};

auto MegaCD::manifest(vector<uint8_t> sector, string location) -> string {
  if(memory::compare(sector.data(), "SEGADISCSYSTEM  ", 16)) return {};

  vector<string> regions;
  string region = slice((const char*)(sector.data() + 0x1f0), 0, 16).trimRight(" ");
  if(!regions) {
    if(region == "JAPAN" ) regions.append("NTSC-J");
    if(region == "EUROPE") regions.append("PAL");
  }
  if(!regions) {
    if(region.find("J")) regions.append("NTSC-J");
    if(region.find("U")) regions.append("NTSC-U");
    if(region.find("E")) regions.append("PAL");
    if(region.find("W")) regions.append("NTSC-J", "NTSC-U", "PAL");
  }
  if(!regions && region.size() == 1) {
    uint8_t field = region.hex();
    if(field & 0x01) regions.append("NTSC-J");
    if(field & 0x04) regions.append("NTSC-U");
    if(field & 0x08) regions.append("PAL");
  }
  if(!regions) {
    regions.append("NTSC-J");
  }

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  return s;
}
