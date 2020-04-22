struct MSX : Cartridge {
  auto name() -> string override { return "MSX"; }
  auto extensions() -> vector<string> override { return {"msx"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto MSX::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto MSX::heuristics(vector<uint8_t>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
