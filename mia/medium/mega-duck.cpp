//Mega Duck cartridges carry no header: no logo/title/checksum block at
//0x0100, and execution starts at 0x0000. There is nothing to analyse beyond
//size, so a banked dump's real MBC (MD0/MD1/MD2/MD20S/MD25S, see
//ares/gb/cartridge/board/megaduck.cpp) is guessed by size below, then a
//hash match against bbbbbr/megaduck-info's game list overrides that guess.
//only the games that guess gets wrong need an entry: MD2 is the size-based
//guess at 64KB and is right for 9 of 11 known 64KB games, so just the 2 real
//MD1 outliers are listed.
//
//note: Duck Adventures drops sprites across the screen after its title
//screen on every board this could try. It also fails on MAME and SameDuck,
//so this looks like a bad dump rather than a board bug - see megaduck2.cpp.
//
//note: MD0/MD20S laptop carts (System ROM, Bilder Lexikon, DataBank,
//QR-Paint, Workboy) need keyboard/RTC/speech peripherals off the serial
//port that do not exist here yet - see megaduck0.cpp.
static const struct { string sha256; string kind; } MegaDuckKnownBanking[] = {
  //MD1: a single 0xb000 write assigns the whole 32KB window, no fixed bank.
  //the outliers at 64KB, where MD2 is the size-based default
  {"d775300664bd04ce7beb23ec55d2dadfbfc395cb1802cb4748e1e8e331bcfae7", "MD1"},  //Puppet Knight
  {"41879f5b4a57f0c9b52f92ae1474562f6a8f12836d939ba7af059dbf92df9245", "MD1"},  //Suleiman's Treasure
};

struct MegaDuck : Cartridge {
  auto name() -> string override { return "Mega Duck"; }
  auto extensions() -> std::vector<string> override { return {"duck", "bin"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom) -> string;
};

auto MegaDuck::load(string location) -> LoadResult {
  std::vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(rom.empty()) return romNotFound;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("board", document["game/board"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return successful;
}

auto MegaDuck::save(string location) -> bool {
  return true;
}

auto MegaDuck::analyze(std::vector<u8>& rom) -> string {
  string board = "Linear";
  //at 32KB there is nothing to bank

  //MD1 and MD2 cap out at fixed sizes by hardware design, and MD2 is the
  //more common of the two at every size both are seen at
  if(rom.size() > 0x8000 && rom.size() <= 0x40000) board = "MegaDuck-MD2";
  if(rom.size() > 0x40000 && rom.size() <= 0x80000) board = "MegaDuck-MD0";
  if(rom.size() > 0x80000) board = "MegaDuck";

  if(rom.size() > 0x8000) {
    for(auto& known : MegaDuckKnownBanking) {
      if(known.sha256 == sha256) { board = {"MegaDuck-", known.kind}; break; }
    }
  }

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  sha256: ", sha256, "\n"};
  s +={"  board:  ", board, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
