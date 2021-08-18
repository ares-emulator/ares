struct SG1000 : Cartridge {
  auto name() -> string override { return "SG-1000"; }
  auto extensions() -> vector<string> override { return {"sg1000", "sg"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto SG1000::load(string location) -> bool {
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
  pak->setAttribute("board",  document["game/board" ].string());  
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }

  return true;
}

auto SG1000::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }

  return true;
}

auto SG1000::analyze(vector<u8>& rom) -> string {
  string hash   = Hash::SHA256(rom).digest();
  string board  = "Linear";

  // Bomberman Special (Taiwan) (DahJee)
  if (hash == "30417fcd412ad281cd6f72c77ecc107b2c13c719bff2d045dde05ea760c757ff") {
    board = "Taiwan-A";
  }

  // Bomberman Special (Taiwan)
  if (hash == "3eff3d6f1f74041f7b732455799d0978ab49724552ff2985f34b76478cd91721") {
    board = "Taiwan-B";
  }

  // Knightmare (Taiwan)
  if (hash == "3106c32c31eb16d9a9534b3975e204a0876c583f48d7a735c325710f03e31f89") {
    board = "Taiwan-A";
  }

  print(string{Medium::name(location), ": ", hash, "\n"});

  string s;
  s += "game\n";
  s +={"  name:  ", Medium::name(location), "\n"};
  s +={"  title: ", Medium::name(location), "\n"};
  s += "  region: NTSC, PAL\n";  //database required to detect region
  s +={"  board:  ", board, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x4000\n";
  s += "      content: Save\n";
  return s;
}
