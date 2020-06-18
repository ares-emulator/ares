struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto manifest(vector<uint8_t> sector, string location) -> string override;
};

auto PlayStation::manifest(vector<uint8_t> sector, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  return s;
}
