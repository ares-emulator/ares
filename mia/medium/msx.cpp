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
  pak->setAttribute("vauspaddle", (bool)document["game/vauspaddle"]);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return true;
}

auto MSX::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto MSX::analyze(vector<u8>& rom) -> string {
  string hash   = Hash::SHA256(rom).digest();
  string board = "Linear";
  bool vauspaddle = false;

  // Roms <= 16KB are most likely (but not always) mirrored
  // This is because MSX looks at 0x4000 for the rom header
  if (rom.size() <= 0x4000) {
    board = "Mirrored";
  }

  // 16KB roms may be mapped at 0x8000 instead of 0x4000
  // We can check for this by checking which range the init
  // and text fields of the cartridge header fall into
  if ((rom.size() == 0x4000) && (rom[0] == 'A') && (rom[1] == 'B')) {
    n16 init = rom[2] | (rom[3] << 8);
    n16 text = rom[8] | (rom[9] << 8);

    bool textHas8000base = text.bit(14, 15) == 2;
    bool hasNoInitVector = init == 0;
    bool initHas8000base = init.bit(14, 15) == 2;
    bool init8000BaseHasRet = initHas8000base && rom[init & (rom.size() - 1)] == 0xC9;

    if (textHas8000base && (hasNoInitVector || init8000BaseHasRet)) {
  	  board = "LinearPage2";
    }
  }  
  
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

  //Special Controllers
  //===================

  // Arkanoid (Japan)
  if (hash == "7100a087369bf03aa117f8103551047d888fc3eb86b339b1af1d51e028aee279") {
    vauspaddle = true;
  }

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s += "  region: NTSC\n";  //database required to detect region
  s +={"  board: ", board, "\n"};
  if (vauspaddle) s += "  vauspaddle\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";

  return s;
}
