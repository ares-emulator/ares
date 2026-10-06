//Gamate cartridges carry no header, so the board type is inferred from the dump
//size the way MAME does it: 16KB and 32KB images fill the window outright, and
//anything larger banks its upper half.
//
//the 4-in-1 board, which also banks the lower half, cannot be told apart from an
//ordinary banked cartridge by size alone; MAME only knows which is which from
//its software list, so it is selected here by matching the known dump.

struct Gamate : Cartridge {
  auto name() -> string override { return "Gamate"; }
  auto extensions() -> std::vector<string> override { return {"gam", "bin"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom) -> string;
};

auto Gamate::load(string location) -> LoadResult {
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

auto Gamate::save(string location) -> bool {
  return true;
}

auto Gamate::analyze(std::vector<u8>& rom) -> string {
  string board = rom.size() > 0x8000 ? "Banked" : "Plain";

  //the only known 4-in-1 cartridge, which banks its lower half as well. nothing
  //in the image distinguishes it from an ordinary banked cartridge, so it is
  //matched by hash exactly as MAME matches it by software list entry
  if(sha256 == "e16d63d17945afd9cc53339b549fcd128e880a7aea73e5b7a7f3933152b0c38d") {
    board = "4-in-1";
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
