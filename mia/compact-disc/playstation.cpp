struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto manifestSector() const -> uint override { return 4; }
  auto manifest(vector<uint8_t> sector, string location) -> string override;
};

auto PlayStation::manifest(vector<uint8_t> sector, string location) -> string {
  string text;
  text.resize(sector.size());
  memory::copy(text.get(), sector.data(), sector.size());

  string region;
  if(text.find("Sony Computer Entertainment Inc.")) region = "NTSC-J";
  if(text.find("Sony Computer Entertainment Amer")) region = "NTSC-U";
  if(text.find("Sony Computer Entertainment Euro")) region = "PAL";
  if(!region) return {};  //audio CD or non-PlayStation CD

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  return s;
}
