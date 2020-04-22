struct MegaDrive : Cartridge {
  auto name() -> string override { return "Mega Drive"; }
  auto extensions() -> vector<string> override { return {"md", "smd", "gen", "bin"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto MegaDrive::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto MegaDrive::heuristics(vector<uint8_t>& data, string location) -> string {
  if(data.size() < 0x200) return {};

  string ramMode = "none";

  uint32_t ramFrom = 0;
  ramFrom |= data[0x01b4] << 24;
  ramFrom |= data[0x01b5] << 16;
  ramFrom |= data[0x01b6] <<  8;
  ramFrom |= data[0x01b7] <<  0;

  uint32_t ramTo = 0;
  ramTo |= data[0x01b8] << 24;
  ramTo |= data[0x01b9] << 16;
  ramTo |= data[0x01ba] <<  8;
  ramTo |= data[0x01bb] <<  0;

  if(!(ramFrom & 1) && !(ramTo & 1)) ramMode = "hi";
  if( (ramFrom & 1) &&  (ramTo & 1)) ramMode = "lo";
  if(!(ramFrom & 1) &&  (ramTo & 1)) ramMode = "word";
  if(data[0x01b0] != 'R' || data[0x01b1] != 'A') ramMode = "none";

  uint32_t ramSize = ramTo - ramFrom + 1;
  if(ramMode == "hi") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "lo") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "word") ramSize = ramTo - ramFrom + 1;
  if(ramMode != "none") ramSize = bit::round(min(0x20000, ramSize));
  if(ramMode == "none") ramSize = 0;

  vector<string> regions;
  string region = slice((const char*)&data[0x1f0], 0, 16).trimRight(" ");
  if(!regions) {
    if(region == "JAPAN" ) regions.append("NTSC-J");
    if(region == "EUROPE") regions.append("PAL");
  }
  if(!regions) {
    if(region.find("J")) regions.append("NTSC-J");
    if(region.find("U")) regions.append("NTSC-U");
    if(region.find("E")) regions.append("PAL");
    if(region.find("W")) regions.append("NTSC-J", "NTSC-U", "PAL");
  }
  if(!regions && region.size() == 1) {
    uint8_t field = region.hex();
    if(field & 0x01) regions.append("NTSC-J");
    if(field & 0x04) regions.append("NTSC-U");
    if(field & 0x08) regions.append("PAL");
  }
  if(!regions) {
    regions.append("NTSC-J");
  }

  string domesticName;
  domesticName.resize(48);
  memory::copy(domesticName.get(), &data[0x0120], domesticName.size());
  for(auto& c : domesticName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(domesticName.find("  ")) domesticName.replace("  ", " ");
  domesticName.strip();

  string internationalName;
  internationalName.resize(48);
  memory::copy(internationalName.get(), &data[0x0150], internationalName.size());
  for(auto& c : internationalName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(internationalName.find("  ")) internationalName.replace("  ", " ");
  internationalName.strip();

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  title:  ", domesticName, "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  s += "  board\n";

  if(domesticName == "Game Genie") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(data.size()), "\n"};
    s += "      content: Program\n";
    s += "    slot\n";
    s += "      type: Mega Drive\n";
  } else if(domesticName == "SONIC & KNUCKLES") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x200000\n";
    s += "      content: Program\n";
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x40000\n";
    s += "      content: Patch\n";
    s += "    slot\n";
    s += "      type: Mega Drive\n";
  } else {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(data.size()), "\n"};
    s += "      content: Program\n";
  }

  if(ramSize && ramMode != "none") {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(ramSize), "\n"};
    s += "      content: Save\n";
    s +={"      mode: ", ramMode, "\n"};
    s +={"      offset: 0x", hex(ramFrom), "\n"};
  }

  s += "  bootable\n";

  return s;
}
