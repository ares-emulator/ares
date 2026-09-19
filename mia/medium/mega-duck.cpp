//Mega Duck cartridges carry no header at all: the Game Boy's logo, title and
//checksum block at 0x0100 does not exist, and execution starts at 0x0000. so
//there is nothing to analyse and nothing to identify a dump by beyond its
//size. MAME's sets store the ROM as a bare numbered .bin.
//
//no mapper has been observed on any known cartridge; everything dumped so far
//is a flat 32KB or 128KB image, so the plain linear board is used.

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
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  sha256: ", sha256, "\n"};
  //at 32KB there is nothing to bank, and a flat board cannot be misdriven by
  //a stray write to 0x0001 or 0xb000
  s +={"  board:  ", rom.size() > 0x8000 ? "MegaDuck" : "Linear", "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
