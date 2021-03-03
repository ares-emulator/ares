struct NeoGeoPocket : Cartridge {
  auto name() -> string override { return "Neo Geo Pocket"; }
  auto extensions() -> vector<string> override { return {"ngp"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
  auto label(vector<u8>& data) -> string;
};

auto NeoGeoPocket::load(string location) -> shared_pointer<vfs::directory> {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.flash"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return {};

  auto pak = shared_pointer{new vfs::directory};
  auto manifest = Cartridge::manifest(rom, location);
  auto document = BML::unserialize(manifest);
  pak->append("manifest.bml",  manifest);
  pak->append("program.flash", rom);
  pak->setAttribute("title", document["game/title"].string());
  return pak;
}

auto NeoGeoPocket::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  return true;
}

auto NeoGeoPocket::heuristics(vector<u8>& data, string location) -> string {
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
  s +={"  title: ", Media::name(location), "\n"};
  s +={"  label: ", label(data), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: Flash\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}

auto NeoGeoPocket::label(vector<u8>& data) -> string {
  string label;
  label.size(12);
  for(u32 index : range(12)) {
    char letter = data[0x24 + index];
    if(letter >= 0x20 && letter <= 0x7e) label.get()[index] = letter;
  }
  return label.strip();
}
