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

  // Bomberman Special (Taiwan) (Chinese Logo) (Unl)
  if (hash == "30417fcd412ad281cd6f72c77ecc107b2c13c719bff2d045dde05ea760c757ff") {
    board = "Taiwan-A";
  }

  // Bomberman Special (Taiwan) (English Logo) (Unl)
  if (hash == "3eff3d6f1f74041f7b732455799d0978ab49724552ff2985f34b76478cd91721") {
    board = "Taiwan-B";
  }

  // Castle, The (Taiwan) (Unl) (Pirate)
  if (hash =="f071cdc34673e48decf2862f70e443558d916bac5e63553e61d0dda50101342a") {
    board = "Taiwan-B";
  }

  // King's Valley (Taiwan)
  if (hash == "490b01e157172cb06a7234d2675df22fbd3dccad3f7d23db11dd5e73a5659c59") {
    board = "Taiwan-A";
  }

  // Knightmare (Taiwan)
  if (hash == "3106c32c31eb16d9a9534b3975e204a0876c583f48d7a735c325710f03e31f89") {
    board = "Taiwan-A";
  }

  // Legend of Kage, The (Taiwan)
  if (hash == "cecc0658956ca46dbc37159a7cd29a12a14b6cb7de91ccea03000cbc5034d0dc") {
    board = "Taiwan-A";
  }

  // Magical Kid Wiz (Taiwan)
  if (hash == "2f443c61e9e6a55dc609de4c789dde4a9243ec6817d0ff4bfb7b6762cabe4e98") {
    board = "Taiwan-B";
  }

  // Pippols (Taiwan)
  if (hash == "2c01a3383d55b21bda938aaac7889ce6ceeed2ef46452535f30ba9d578cc54e1") {
    board = "Taiwan-A";
  }

  // Rally-X (Taiwan) (Chinese Logo)
  if (hash == "952c8e1788590b1d09c23368e133e76d3db59444787f1afa90be6ab075049734") {
    board = "Taiwan-A";
  }

  // Rally-X (Taiwan) (English Logo) 
  if (hash == "f10416a655b2de101ad324695831468de7d9ae5690bdc1106f72c0e78f3fd9fb") {
    board = "Taiwan-B";
  }

  // Road Fighter (Taiwan) (Chinese Logo)
  if (hash == "ead9e34e1b4f3f029f99dd0446641056e30a28596b30500bcd5134ac5c968633") {
    board = "Taiwan-A";
  }

  // Road Fighter (Taiwan) (English Logo)
  if (hash == "f604b1374673b28e289ecacb85514c6b33a7516b82c8f53c2ec979eada505a60") {
    board = "Taiwan-B";
  }

  // Star Soldier (Taiwan)
  if (hash == "dfcc8ac91e6f8004b9cf3d78b3879fcee1f071a97da00578ecd389bf1c954c1d") {
    board = "Taiwan-A";
  }

  // Tank Battalion (Taiwan)
  if (hash == "1a74c2a02adf19104a319a507d88181337eb5314c558d6696ddcdddf243681e9") {
    board = "Taiwan-A";
  }

  // TwinBee (Taiwan) 
  if (hash == "24f25236f743b5aab10dba38b182e52028e99c0093f004406c6973510d2b0875") {
    board = "Taiwan-A";
  }

  // Yie Ar Kung-Fu II (Taiwan)
  if (hash == "eb34e17ed22f0f23a13b39b3c78ecd9fe608d3f9ebd55f025d9812dfbbbf3e2f") {
    board = "Taiwan-A";
  }


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
