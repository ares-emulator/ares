struct Mega32X : Cartridge {
  auto name() -> string override { return "Mega 32X"; }
  auto extensions() -> vector<string> override { return {"32x"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto Mega32X::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return false;

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title",    document["game/title"].string());
  pak->setAttribute("region",   document["game/region"].string());
  pak->setAttribute("bootable", true);
  pak->setAttribute("32x",      true);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Pak::load(node, ".ram");
    if(auto fp = pak->read("save.ram")) {
      fp->setAttribute("mode",   node["mode"].string());
      fp->setAttribute("offset", node["offset"].natural());
    }
  }

  return true;
}

auto Mega32X::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Pak::save(node, ".ram");
  }

  return true;
}

auto Mega32X::analyze(vector<u8>& rom) -> string {
  if(rom.size() < 0x800) return {};

  string ramMode = "none";

  u32 ramFrom = 0;
  ramFrom |= rom[0x01b4] << 24;
  ramFrom |= rom[0x01b5] << 16;
  ramFrom |= rom[0x01b6] <<  8;
  ramFrom |= rom[0x01b7] <<  0;

  u32 ramTo = 0;
  ramTo |= rom[0x01b8] << 24;
  ramTo |= rom[0x01b9] << 16;
  ramTo |= rom[0x01ba] <<  8;
  ramTo |= rom[0x01bb] <<  0;

  if(!(ramFrom & 1) && !(ramTo & 1)) ramMode = "hi";
  if( (ramFrom & 1) &&  (ramTo & 1)) ramMode = "lo";
  if(!(ramFrom & 1) &&  (ramTo & 1)) ramMode = "word";
  if(rom[0x01b0] != 'R' || rom[0x01b1] != 'A') ramMode = "none";

  u32 ramSize = ramTo - ramFrom + 1;
  if(ramMode == "hi") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "lo") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "word") ramSize = ramTo - ramFrom + 1;
  if(ramMode != "none") ramSize = bit::round(min(0x20000, ramSize));
  if(ramMode == "none") ramSize = 0;

  vector<string> regions;
  string region = slice((const char*)&rom[0x01f0], 0, 16).trimRight(" ");
  if(!regions) {
    if(region == "JAPAN ") regions.append("NTSC-J");
    if(region == "EUROPE") regions.append("PAL");
  }
  if(!regions) {
    if(region.find("J")) regions.append("NTSC-J");
    if(region.find("U")) regions.append("NTSC-U");
    if(region.find("E")) regions.append("PAL");
    if(region.find("W")) regions.append("NTSC-J", "NTSC-U", "PAL");
  }
  if(!regions && region.size() == 1) {
    u8 field = region.hex();
    if(field & 0x01) regions.append("NTSC-J");
    if(field & 0x04) regions.append("NTSC-U");
    if(field & 0x08) regions.append("PAL");
  }
  if(!regions) {
    regions.append("NTSC-J");
  }

  string domesticName;
  domesticName.resize(48);
  memory::copy(domesticName.get(), &rom[0x0120], domesticName.size());
  for(auto& c : domesticName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(domesticName.find("  ")) domesticName.replace("  ", " ");
  domesticName.strip();

  string internationalName;
  internationalName.resize(48);
  memory::copy(internationalName.get(), &rom[0x0150], internationalName.size());
  for(auto& c : internationalName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(internationalName.find("  ")) internationalName.replace("  ", " ");
  internationalName.strip();

  string s;
  s += "game\n";
  s +={"  name:   ", Pak::name(location), "\n"};
  s +={"  title:  ", Pak::name(location), "\n"};
  s +={"  label:  ", domesticName, "\n"};
  s +={"  label:  ", internationalName, "\n"};
  s += "  region: NTSC-J\n";
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  if(!ramSize || ramMode == "none") return s;
  s += "    memory\n";
  s += "      type: RAM\n";
  s +={"      size: 0x", hex(ramSize), "\n"};
  s += "      content: Save\n";
  s +={"      mode: ", ramMode, "\n"};
  s +={"      offset: 0x", hex(ramFrom), "\n"};
  return s;
}
