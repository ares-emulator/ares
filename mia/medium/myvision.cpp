
// TODO: Add Mame Zip subport.

struct MyVision : Cartridge {
  auto name() -> string override { return "MyVision"; }
  auto extensions() -> std::vector<string> override { return {"myv"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom) -> string;
};

auto MyVision::load(string location) -> LoadResult {
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
  pak->setAttribute("title",  document["game/title"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return successful;
}

auto MyVision::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto MyVision::analyze(std::vector<u8>& rom) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  sha256: ", sha256, "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
