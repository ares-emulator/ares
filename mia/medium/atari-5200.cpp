struct Atari5200 : Cartridge {
  auto name() -> string override { return "Atari 5200"; }
  auto extensions() -> std::vector<string> override { return {"a52", "bin", "car"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(const std::vector<u8>& rom) -> string;
};

auto Atari5200::load(string location) -> LoadResult {
  std::vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(rom.empty()) return romNotFound;

  if(rom.size() >= 4 && rom[0] == 'C' && rom[1] == 'A' && rom[2] == 'R' && rom[3] == 'T') {
    return invalidROM;
  }

  sha256 = Hash::SHA256(rom).digest();
  if(rom.size() != 32_KiB) {
    // Raw 16 KiB images are ambiguous between one-chip and two-chip boards.
    // Smaller images, headered .car files, and additional board layouts belong
    // to the mapper child so this scaffold never guesses a wiring layout.
    return invalidROM;
  }

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board", document["game/board"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom", rom);
  return successful;
}

auto Atari5200::save(string location) -> bool {
  return true;
}

auto Atari5200::analyze(const std::vector<u8>& rom) -> string {
  string manifest;
  manifest += "game\n";
  manifest +={"  name:   ", Medium::name(location), "\n"};
  manifest +={"  title:  ", Medium::name(location), "\n"};
  manifest += "  region: NTSC\n";
  manifest +={"  sha256: ", sha256, "\n"};
  manifest += "  board:  Linear32K\n";
  manifest += "    memory\n";
  manifest += "      type: ROM\n";
  manifest +={"      size: 0x", hex(rom.size()), "\n"};
  manifest += "      content: Program\n";
  return manifest;
}
