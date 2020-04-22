struct GameBoyAdvance : Cartridge {
  auto name() -> string override { return "Game Boy Advance"; }
  auto extensions() -> vector<string> override { return {"gba"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto GameBoyAdvance::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto GameBoyAdvance::heuristics(vector<uint8_t>& data, string location) -> string {
  vector<string> identifiers = {
    "SRAM_V",
    "SRAM_F_V",
    "EEPROM_V",
    "FLASH_V",
    "FLASH512_V",
    "FLASH1M_V",
  };

  vector<string> list;
  for(auto& identifier : identifiers) {
    for(int n : range(data.size() - 16)) {
      if(!memory::compare(&data[n], identifier.data(), identifier.size())) {
        auto p = (const char*)&data[n + identifier.size()];
        if(p[0] >= '0' && p[0] <= '9'
        && p[1] >= '0' && p[1] <= '9'
        && p[2] >= '0' && p[2] <= '9'
        ) {
          char text[16];
          memory::copy(text, &data[n], identifier.size() + 3);
          text[identifier.size() + 3] = 0;
          if(!list.find(text)) list.append(text);
        }
      }
    }
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

  if(list) {
    if(list.first().beginsWith("SRAM_V") || list.first().beginsWith("SRAM_F_V")) {
      s += "    memory\n";
      s += "      type: RAM\n";
      s += "      size: 0x8000\n";
      s += "      content: Save\n";
    }

    if(list.first().beginsWith("EEPROM_V")) {
      s += "    memory\n";
      s += "      type: EEPROM\n";
      s += "      size: 0x0\n";  //auto-detected
      s += "      content: Save\n";
    }

    if(list.first().beginsWith("FLASH_V") || list.first().beginsWith("FLASH512_V")) {
      s += "    memory\n";
      s += "      type: Flash\n";
      s += "      size: 0x10000\n";
      s += "      content: Save\n";
      s += "      manufacturer: Macronix\n";
    }

    if(list.first().beginsWith("FLASH1M_V")) {
      s += "    memory\n";
      s += "      type: Flash\n";
      s += "      size: 0x20000\n";
      s += "      content: Save\n";
      s += "      manufacturer: Macronix\n";
    }
  }

  return s;
}
