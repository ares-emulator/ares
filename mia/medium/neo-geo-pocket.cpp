struct NeoGeoPocket : Cartridge {
  auto name() -> string override { return "Neo Geo Pocket"; }
  auto extensions() -> vector<string> override { return {"ngp"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
  auto label(vector<u8>& data) -> string;
};

auto NeoGeoPocket::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.flash"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return false;

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title", document["game/title"].string());
  pak->append("manifest.bml",  manifest);
  pak->append("program.flash", rom);

  Pak::load("program.flash", ".sav");

  return true;
}

auto NeoGeoPocket::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  Pak::save("program.flash", ".sav");

  return true;
}

auto NeoGeoPocket::analyze(vector<u8>& rom) -> string {
  //expand ROMs that are smaller than valid flash chip sizes (homebrew games)
       if(rom.size() <= 0x080000) rom.resize(0x080000);  // 4mbit
  else if(rom.size() <= 0x100000) rom.resize(0x100000);  // 8mbit
  else if(rom.size() <= 0x200000) rom.resize(0x200000);  //16mbit
  else if(rom.size() <= 0x280000) rom.resize(0x280000);  //16mbit +  4mbit
  else if(rom.size() <= 0x300000) rom.resize(0x300000);  //16mbit +  8mbit
  else if(rom.size() <= 0x400000) rom.resize(0x400000);  //16mbit + 16mbit

  string s;
  s += "game\n";
  s +={"  name:  ", Medium::name(location), "\n"};
  s +={"  title: ", Medium::name(location), "\n"};
  s +={"  label: ", label(rom), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: Flash\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  return s;
}

auto NeoGeoPocket::label(vector<u8>& rom) -> string {
  string label;
  label.size(12);
  for(u32 index : range(12)) {
    char letter = rom[0x24 + index];
    if(letter >= 0x20 && letter <= 0x7e) label.get()[index] = letter;
  }
  return label.strip();
}
