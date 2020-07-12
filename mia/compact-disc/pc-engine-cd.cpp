struct PCEngineCD : CompactDisc {
  auto name() -> string override { return "PC Engine CD"; }
  auto manifestSector() const -> uint override { return 0; }
  auto manifest(vector<uint8_t> sector, string location) -> string override;
};

auto PCEngineCD::manifest(vector<uint8_t> sector, string location) -> string {
  //yes, "Electronics" is spelled incorrectly in actual PC Engine CD games ...
  if(memory::compare(sector.data() + 0x264, "NEC Home Electoronics", 21)) return {};

  //note: there is no method to determine the region for PC Engine CDs ...
  string region = "NTSC-J";

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  return s;
}
