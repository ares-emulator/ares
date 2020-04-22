struct BSMemory : Cartridge {
  auto name() -> string override { return "BS Memory"; }
  auto extensions() -> vector<string> override { return {"bs"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto BSMemory::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  append(data, {location, "program.flash"});
  return data;
}

auto BSMemory::heuristics(vector<uint8_t>& data, string location) -> string {
  if(data.size() < 0x8000) return {};

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: Flash\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
