struct ColecoVision : Cartridge {
  auto name() -> string override { return "ColecoVision"; }
  auto extensions() -> std::vector<string> override { return {"col"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom) -> string;
};

auto ColecoVision::load(string location) -> LoadResult {
  std::vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(rom.empty()) return romNotFound;

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("board",  document["game/board" ].string());
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return successful;
}

auto ColecoVision::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto ColecoVision::analyze(std::vector<u8>& rom) -> string {
  string hash   = Hash::SHA256(rom).digest();
  string board  = "coleco";
  
  //megacart (homebrew)
  if(rom.size() > 0x8000) {
    board  = "megacart";
  }

  //31 in 1
  if(hash == "8c0510916f990a69b4699d70d47e09a13e9da12a29109332964e77000a5cf875") {
    board  = "xin1";
  }

  // 63 in 1
  if(hash == "74138e164b0e60426a9dcc71eb37e11be60f7d8794f5aaa6e6371b2475dace1a") {
    board  = "xin1";
  }

  string s;
  s += "game\n";
  s +={"  name:  ", Medium::name(location), "\n"};
  s +={"  title: ", Medium::name(location), "\n"};
  s += "  region: NTSC, PAL\n";  //database required to detect region
  s +={"  sha256: ", hash, "\n"};
  s +={"  board:  ", board, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
