struct Atari2600 : Cartridge {
  auto name() -> string override { return "Atari 2600"; }
  auto extensions() -> vector<string> override { return {"a26", "bin"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom, string location) -> string;

  auto match(vector<u8>& rom, vector<u8> pattern, u8 target_matches = 1) -> bool;
};

auto Atari2600::load(string location) -> LoadResult {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return romNotFound;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = Medium::manifestDatabase(sha256);
  if(!manifest) manifest = analyze(rom, location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board",  document["game/board"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return successful;
}

auto Atari2600::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto Atari2600::analyze(vector<u8>& rom, string location) -> string {
  string board = "Linear";

  // TODO: More heuristics for more mapper types
  // The below should work for most of the commercially released titles at least...
  bool maybeCV = match(rom, { 0x9d, 0xff, 0xf3 }) || match(rom, { 0x99, 0x00, 0xf4 });
  bool maybeF8 = match(rom, { 0x8d, 0xf9, 0x1f }, 2) || match(rom, { 0x8d, 0xf9, 0xff }, 2);
  bool maybeFE = match(rom, { 0x20, 0x00, 0xd0, 0xc6, 0xc5 }) || match(rom, { 0x20, 0xc3, 0xf8, 0xa5, 0x82 }) ||
                 match(rom, {0xd0, 0xfB, 0x20, 0x73, 0xfe}) || match(rom, {0x20, 0x00, 0xf0, 0x84, 0xd6});
  bool maybeE0 = match(rom, { 0x8D, 0xe0, 0x1f }) || match(rom, { 0x8d, 0xe0, 0x5f }) ||
                 match(rom, { 0x8d, 0xe9, 0xff }) || match(rom, { 0x0c, 0xe0, 0x1f }) || match(rom, { 0xad, 0xe0, 0x1f }) ||
                 match(rom, { 0xad, 0xe9, 0xff }) || match(rom, { 0xad, 0xed, 0xff }) || match(rom, { 0xad, 0xf3, 0xbf });
  bool maybe3F = match(rom, { 0x85, 0x3f }, 2);
  bool maybeE7 = match(rom, { 0xad, 0xe2, 0xff }) || match(rom, { 0xad, 0xe5, 0xff }) || match(rom, { 0xad, 0xe5, 0x1f }) ||
                 match(rom, { 0xad, 0xe7, 0x1f }) || match(rom, { 0x0c, 0xe7, 0x1f }) || match(rom, { 0x8d, 0xe7, 0xff }) ||
                 match(rom, { 0x8d, 0xe7, 0x1f }) || match(rom, { 0xad, 0xe4, 0xff }) || match(rom, { 0xad, 0xe5, 0xff }) ||
                 match(rom, { 0xad, 0xe6, 0xff });
  bool maybeUA = match(rom, { 0x8d, 0x40, 0x02 }) || match(rom, { 0xad, 0x40, 0x02 }) || match(rom, { 0xbd, 0x1f, 0x02 }) ||
                 match(rom, { 0x2c, 0xc0, 0x02 }) || match(rom, { 0x8D, 0xc0, 0x02 }) || match(rom, { 0xad, 0xc0, 0x02 });

  if(rom.size() == 2_KiB && maybeCV) board = "Commavid";
  else if(rom.size() == 4_KiB && maybeCV) board = "Commavid";
  else if(rom.size() == 8_KiB) {
    if(maybeE0) board = "ParkerBros8k";
    else if(maybe3F) board = "Tigervision";
    else if(maybeUA) board = "UA8k";
    else if(maybeFE && !maybeF8) board = "Activision8k";
    else board = "Atari8k";
  } else if(rom.size() == 12_KiB) {
    if(maybeE7) board = "MNetwork16k";
    else board = "CbsRam8k";
  } else if(rom.size() == 16_KiB) {
    if(maybeE7) board = "MNetwork16k";
    else board = "Atari16k";
  } else if(rom.size() == 32_KiB) {
    if(maybe3F) board = "Tigervision";
    else board = "Atari32k";
  }

  // For accurate region detection, a database is required
  // but we can make some educated guesses based on filename
  string region = "NTSC";
  if(location.ifind("(Europe)")) region = "PAL";
  if(location.ifind("(PAL)")) region = "PAL";

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};  //database required to detect region
  s +={"  sha256: ", sha256, "\n"};
  s +={"  board:  ", board, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";

  return s;
}

auto Atari2600::match(vector<u8>& rom, vector<u8> pattern, u8 target_matches) -> bool {
  u8 matches = 0;

  for (int romIndex = 0; romIndex + pattern.size() <= rom.size(); ++romIndex) {
    int patternIndex = 0;

    // Loop until either the end of the pattern, or don't find a match
    for (patternIndex = 0; patternIndex < pattern.size(); ++patternIndex) {
      if (rom[romIndex + patternIndex] != pattern[patternIndex]) break;
    }

    if (patternIndex == pattern.size()) {
      if(++matches == target_matches) break;
      romIndex += pattern.size();
    }
  }

  // Return true if we found the desired number of matches
  return matches == target_matches;
}
