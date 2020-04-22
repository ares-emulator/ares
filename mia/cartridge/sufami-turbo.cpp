struct SufamiTurbo : Cartridge {
  auto name() -> string override { return "Sufami Turbo"; }
  auto extensions() -> vector<string> override { return {"st"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto SufamiTurbo::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto SufamiTurbo::heuristics(vector<uint8_t>& data, string location) -> string {
  if(data.size() < 0x20000) return {};

  uint romSize = data[0x36] * 128_KiB;
  uint ramSize = data[0x37] *   2_KiB;

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";

  if(ramSize) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(ramSize), "\n"};
    s += "      content: Save\n";
  }

  return s;
}
