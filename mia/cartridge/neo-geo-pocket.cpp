struct NeoGeoPocket : Cartridge {
  auto name() -> string override { return "Neo Geo Pocket"; }
  auto extensions() -> vector<string> override { return {"ngp"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
  auto title(vector<uint8_t>& data) -> string;
};

auto NeoGeoPocket::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.flash"});
  return data;
}

auto NeoGeoPocket::heuristics(vector<uint8_t>& data, string location) -> string {
  //expand ROMs that are smaller than valid flash chip sizes (homebrew games)
       if(data.size() <= 0x080000) data.resize(0x080000);  // 4mbit
  else if(data.size() <= 0x100000) data.resize(0x100000);  // 8mbit
  else if(data.size() <= 0x200000) data.resize(0x200000);  //16mbit
  else if(data.size() <= 0x280000) data.resize(0x280000);  //16mbit +  4mbit
  else if(data.size() <= 0x300000) data.resize(0x300000);  //16mbit +  8mbit
  else if(data.size() <= 0x400000) data.resize(0x400000);  //16mbit + 16mbit

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s +={"  title: ", title(data), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: Flash\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}

auto NeoGeoPocket::title(vector<uint8_t>& data) -> string {
  string title;
  title.size(12);
  for(uint index : range(12)) {
    char letter = data[0x24 + index];
    if(letter >= 0x20 && letter <= 0x7e) title.get()[index] = letter;
  }
  return title.strip();
}
