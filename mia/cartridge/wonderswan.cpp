struct WonderSwan : Cartridge {
  auto name() -> string override { return "WonderSwan"; }
  auto extensions() -> vector<string> override { return {"ws"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto WonderSwan::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto WonderSwan::heuristics(vector<uint8_t>& data, string location) -> string {
  if(data.size() < 0x10000) return {};

  auto metadata = &data[data.size() - 16];

  bool color = metadata[7];

  string ramType;
  uint ramSize = 0;
  switch(metadata[11]) {
  case 0x01: ramType = "RAM";    ramSize =    8 * 1024; break;
  case 0x02: ramType = "RAM";    ramSize =   32 * 1024; break;
  case 0x03: ramType = "RAM";    ramSize =  128 * 1024; break;
  case 0x04: ramType = "RAM";    ramSize =  256 * 1024; break;
  case 0x05: ramType = "RAM";    ramSize =  512 * 1024; break;
  case 0x10: ramType = "EEPROM"; ramSize =  128;        break;
  case 0x20: ramType = "EEPROM"; ramSize = 2048;        break;
  case 0x50: ramType = "EEPROM"; ramSize = 1024;        break;
  }

  bool orientation = metadata[12] & 1;  //0 = horizontal; 1 = vertical
  bool hasRTC = metadata[13] & 1;

  string s;
  s += "game\n";
  s +={"  name:        ", Media::name(location), "\n"};
  s +={"  label:       ", Media::name(location), "\n"};
  s +={"  orientation: ", !orientation ? "horizontal" : "vertical", "\n"};
  s += "  board\n";

  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";

  if(ramType && ramSize) {
    s += "    memory\n";
    s +={"      type: ", ramType, "\n"};
    s +={"      size: 0x", hex(ramSize), "\n"};
    s += "      content: Save\n";
  }

  if(hasRTC) {
    s += "    memory\n";
    s += "      type: RTC\n";
    s += "      size: 0x10\n";
    s += "      content: Time\n";
  }

  return s;
}
