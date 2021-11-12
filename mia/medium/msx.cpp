struct MSX : Cartridge {
  auto name() -> string override { return "MSX"; }
  auto extensions() -> vector<string> override { return {"msx"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto MSX::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return false;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = Medium::manifestDatabase(sha256);
  if(!manifest) manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board",  document["game/board"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return true;
}

auto MSX::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto MSX::analyze(vector<u8>& rom) -> string {
  string board = "Linear";

  // If the rom is too big to be linear, attempt to guess the mapper
  // based on the number of times specific banking instructions occur 
  // in the binary
  if(rom.size() > 0x10000) {
    const auto ASC16      = 0;
    const auto ASC8       = 1;
    const auto KONAMI     = 2;
    const auto KONAMI_SCC = 3;
    int bankingCount[4] = {0,0,0,0}; 

    for(auto i : range(rom.size() - 3)) {
      if (rom[i] == 0x32) { // ld(nn),a 
        u16 value = rom[i + 1] + (rom[i + 2] << 8);
        switch (value) {
          case 0x5000: case 0x9000: case 0xb000: bankingCount[KONAMI_SCC]++; break;
          case 0x4000: case 0x8000: case 0xa000: bankingCount[KONAMI]++; break;
          case 0x6800: case 0x7800: bankingCount[ASC8]++; break;
          case 0x6000: bankingCount[KONAMI]++; bankingCount[ASC8]++; bankingCount[ASC16]++; break;
          case 0x7000: bankingCount[KONAMI_SCC]++; bankingCount[ASC8]++; bankingCount[ASC16]++; break;
          case 0x77ff: bankingCount[ASC16]++; break;
        }
      }
    }

    auto mapperNum = 0, max = 0;
    for(auto i : range(4)) {
      if (bankingCount[i] > max) {max = bankingCount[i]; mapperNum = i;}
    }

    switch(mapperNum) {
      case 0: board = "ASC16"; break;
      case 1: board = "ASC8"; break;
      case 2: board = "Konami"; break;
      case 3: board = "KonamiSCC"; break; 
    }
  }

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s += "  region: NTSC\n";  //database required to detect region
  s +={"  board: ", board, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";

  print(s);
  return s;
}
