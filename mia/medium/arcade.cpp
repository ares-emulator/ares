struct Arcade : Mame {
  auto name() -> string override { return "Arcade"; }
  auto extensions() -> vector<string> override { return {}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;

  Markup::Node info;
};

auto Arcade::load(string location) -> bool {
  info = BML::unserialize(manifestDatabaseArcade(Medium::name(location)));
  if(!info) return false;

  vector<u8> rom = loadRoms(location, info, "maincpu");
  if(!rom) return false;

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("board",  document["game/board" ].string());  
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return true;
}

auto Arcade::save(string location) -> bool {
  return true;
}

auto Arcade::analyze(vector<u8>& rom) -> string {
  string hash   = Hash::SHA256(rom).digest();

  string s;
  s += "game\n";
  s +={"  name:  ", Medium::name(location), "\n"};
  s +={"  title:  ", (info ? info["game/title"].string() : Medium::name(location)), "\n"};
  s +={"  board:  ", (info ? info["game/board"].string() : "Arcade"), "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
