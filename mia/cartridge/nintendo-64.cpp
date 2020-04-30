struct Nintendo64 : Cartridge {
  auto name() -> string override { return "Nintendo 64"; }
  auto extensions() -> vector<string> override { return {"n64", "v64", "z64"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto Nintendo64::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto Nintendo64::heuristics(vector<uint8_t>& data, string location) -> string {
  if(data.size() < 4) {
    //too small
    return {};
  } else if(data[0] == 0x80 && data[1] == 0x37 && data[2] == 0x12 && data[3] == 0x40) {
    //big endian
  } else if(data[0] == 0x37 && data[1] == 0x80 && data[2] == 0x40 && data[3] == 0x12) {
    //byte-swapped
    for(uint index = 0; index < data.size(); index += 2) {
      uint8_t d0 = data[index + 0];
      uint8_t d1 = data[index + 1];
      data[index + 0] = d1;
      data[index + 1] = d0;
    }
  } else if(data[0] == 0x40 && data[1] == 0x12 && data[2] == 0x37 && data[3] == 0x80) {
    //little endian
    for(uint index = 0; index < data.size(); index += 4) {
      uint8_t d0 = data[index + 0];
      uint8_t d1 = data[index + 1];
      uint8_t d2 = data[index + 2];
      uint8_t d3 = data[index + 3];
      data[index + 0] = d3;
      data[index + 1] = d2;
      data[index + 2] = d1;
      data[index + 3] = d0;
    }
  } else {
    //unrecognized
    return {};
  }

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
