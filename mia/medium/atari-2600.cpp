struct Atari2600 : Cartridge {
  auto name() -> string override { return "Atari 2600"; }
  auto extensions() -> std::vector<string> override { return {"a26", "bin"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;

private:
  auto analyze(std::vector<u8>& rom) -> string;

  auto identifyBoard(std::vector<u8>& rom) -> string;
  auto identify8KiBBoard(std::vector<u8>& rom) -> string;
  auto identify12KiBBoard(std::vector<u8>& rom) -> string;
  auto identify16KiBBoard(std::vector<u8>& rom) -> string;
  auto identify32KiBBoard(std::vector<u8>& rom) -> string;
  auto identify64KiBBoard(std::vector<u8>& rom) -> string;
  auto identify128KiBBoard(std::vector<u8>& rom) -> string;
  auto identify256KiBBoard(std::vector<u8>& rom) -> string;
  auto identify512KiBBoard(std::vector<u8>& rom) -> string;

  auto hasCommavidSignature(std::vector<u8>& rom) -> bool;
  auto hasAtariF8Signature(std::vector<u8>& rom) -> bool;
  auto hasActivisionFESignature(std::vector<u8>& rom) -> bool;
  auto hasParkerBrosE0Signature(std::vector<u8>& rom) -> bool;
  auto has3EPlusSignature(std::vector<u8>& rom) -> bool;
  auto has3EXSignature(std::vector<u8>& rom) -> bool;
  auto has3ESignature(std::vector<u8>& rom) -> bool;
  auto has3FSignature(std::vector<u8>& rom) -> bool;
  auto hasMNetwork8KiBSignature(std::vector<u8>& rom) -> bool;
  auto hasMNetworkSignature(std::vector<u8>& rom) -> bool;
  auto hasUASignature(std::vector<u8>& rom) -> bool;
  auto hasJVPSignature(std::vector<u8>& rom) -> bool;
  auto hasEconoBanking0840Signature(std::vector<u8>& rom) -> bool;
  auto hasWicksteadSignature(std::vector<u8>& rom) -> bool;
  auto hasJaneSignature(std::vector<u8>& rom) -> bool;
  auto hasParkerBros03E0Signature(std::vector<u8>& rom) -> bool;
  auto hasCPUWiz4KSCSignature(std::vector<u8>& rom) -> bool;
  auto hasDPCPlusSignature(std::vector<u8>& rom) -> bool;
  auto hasAmigaFCSignature(std::vector<u8>& rom) -> bool;
  auto has4A50Signature(std::vector<u8>& rom) -> bool;
  auto hasEFSCSignature(std::vector<u8>& rom) -> bool;
  auto hasEFSignature(std::vector<u8>& rom) -> bool;
  auto hasDFSCSignature(std::vector<u8>& rom) -> bool;
  auto hasDFSignature(std::vector<u8>& rom) -> bool;
  auto hasBFSCSignature(std::vector<u8>& rom) -> bool;
  auto hasBFSignature(std::vector<u8>& rom) -> bool;
  auto hasMDMSignature(std::vector<u8>& rom) -> bool;
  auto hasX07Signature(std::vector<u8>& rom) -> bool;
  auto hasSuperbankingSignature(std::vector<u8>& rom) -> bool;

  auto matchAny(std::vector<u8>& rom, std::initializer_list<std::vector<u8>> patterns,
    u8 targetMatches = 1) -> bool;
  auto match(std::vector<u8>& rom, std::vector<u8> pattern, u8 target_matches = 1) -> bool;
  auto hasTailMarker(std::vector<u8>& rom, std::vector<u8> marker) -> bool;
  auto hasRepeatedRamWindow(std::vector<u8>& rom) -> bool;
  auto hasSaraRamLayout(std::vector<u8>& rom) -> bool;
  auto normalizeWDSW(std::vector<u8>& rom) -> void;
};

auto Atari2600::load(string location) -> LoadResult {
  std::vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  normalizeWDSW(rom);
  if(rom.empty()) return romNotFound;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = Medium::manifestDatabase(sha256);
  if(!manifest) manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
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

auto Atari2600::analyze(std::vector<u8>& rom) -> string {
  auto board = identifyBoard(rom);

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

auto Atari2600::identifyBoard(std::vector<u8>& rom) -> string {
  auto size = rom.size();
  if(size >= 8_KiB && size <= 64_KiB && size % 1_KiB == 0 && has3EPlusSignature(rom)) return "3E+";
  if(size >= 8_KiB && size <= 512_KiB && size % 4_KiB == 0 && hasMDMSignature(rom)) return "MDM";
  if(size == 10_KiB + 255 || size == 10_KiB + 256) return "DPC";
  if(size == 29_KiB && hasDPCPlusSignature(rom))   return "DPC+";
  if(size == 2_KiB && hasCommavidSignature(rom))   return "Commavid";
  if(size == 4_KiB && hasCommavidSignature(rom))   return "Commavid";
  if(size == 4_KiB && hasCPUWiz4KSCSignature(rom)) return "4KSC";
  if(size == 4_KiB && hasAmigaFCSignature(rom))    return "AmigaFC";
  if(size == 8_KiB)                                return identify8KiBBoard(rom);
  if(size == 12_KiB)                               return identify12KiBBoard(rom);
  if(size == 16_KiB)                               return identify16KiBBoard(rom);
  if(size == 32_KiB)                               return identify32KiBBoard(rom);
  if(size == 64_KiB)                               return identify64KiBBoard(rom);
  if(size == 128_KiB)                              return identify128KiBBoard(rom);
  if(size == 256_KiB)                              return identify256KiBBoard(rom);
  if(size == 512_KiB)                              return identify512KiBBoard(rom);
                                                   return "Linear";
}

auto Atari2600::identify8KiBBoard(std::vector<u8>& rom) -> string {
  if(hasSaraRamLayout(rom))                                      return "Atari8kSC";
  if(hasParkerBrosE0Signature(rom))                              return "ParkerBros8k";
  if(has3EXSignature(rom))                                       return "3EX";
  if(has3ESignature(rom))                                        return "3E";
  if(has3FSignature(rom))                                        return "Tigervision";
  if(hasUASignature(rom))                                        return "UA8k";
  if(hasJVPSignature(rom))                                       return "JVP";
  if(hasActivisionFESignature(rom) && !hasAtariF8Signature(rom)) return "Activision8k";
  if(hasEconoBanking0840Signature(rom))                          return "EconoBanking";
  if(hasMNetwork8KiBSignature(rom))                              return "MNetwork";
  if(hasWicksteadSignature(rom))                                 return "Wickstead";
  if(hasAmigaFCSignature(rom))                                   return "AmigaFC";
  if(hasParkerBros03E0Signature(rom))                            return "ParkerBros03E0";
                                                                 return "Atari8k";
}

auto Atari2600::identify12KiBBoard(std::vector<u8>& rom) -> string {
  if(hasMNetworkSignature(rom)) return "MNetwork";
                                return "CbsRamPlus";
}

auto Atari2600::identify16KiBBoard(std::vector<u8>& rom) -> string {
  if(hasSaraRamLayout(rom))     return "Atari16kSC";
  if(hasMNetworkSignature(rom)) return "MNetwork";
  if(hasAmigaFCSignature(rom))  return "AmigaFC";
  if(has3EXSignature(rom))      return "3EX";
  if(has3ESignature(rom))       return "3E";
  if(hasJaneSignature(rom))     return "Jane";
                                return "Atari16k";
}

auto Atari2600::identify32KiBBoard(std::vector<u8>& rom) -> string {
  if(hasDPCPlusSignature(rom))       return "DPC+";
  if(hasSaraRamLayout(rom))          return "Atari32kSC";
  if(has3EXSignature(rom))           return "3EX";
  if(has3ESignature(rom))            return "3E";
  if(has3FSignature(rom))            return "Enhanced3F";
  if(hasAmigaFCSignature(rom))       return "AmigaFC";
                                     return "Atari32k";
}

auto Atari2600::identify64KiBBoard(std::vector<u8>& rom) -> string {
  if(has3EXSignature(rom))  return "3EX";
  if(has3ESignature(rom))   return "3E";
  if(has3FSignature(rom))   return "Enhanced3F";
  if(has4A50Signature(rom)) return "4A50";
  if(hasEFSCSignature(rom)) return "EFSC";
  if(hasEFSignature(rom))   return "EF";
  if(hasX07Signature(rom))  return "X07";
                            return "Linear";
}

auto Atari2600::identify128KiBBoard(std::vector<u8>& rom) -> string {
  if(has3EXSignature(rom))          return "3EX";
  if(has3ESignature(rom))           return "3E";
  if(hasDFSCSignature(rom))         return "DFSC";
  if(hasDFSignature(rom))           return "DF";
  if(has3FSignature(rom))           return "Enhanced3F";
  if(has4A50Signature(rom))         return "4A50";
  if(hasSuperbankingSignature(rom)) return "Superbanking";
                                    return "Linear";
}

auto Atari2600::identify256KiBBoard(std::vector<u8>& rom) -> string {
  if(has3EXSignature(rom))          return "3EX";
  if(has3ESignature(rom))           return "3E";
  if(hasBFSCSignature(rom))         return "BFSC";
  if(hasBFSignature(rom))           return "BF";
  if(has3FSignature(rom))           return "Enhanced3F";
  if(hasSuperbankingSignature(rom)) return "Superbanking";
                                    return "Linear";
}

auto Atari2600::identify512KiBBoard(std::vector<u8>& rom) -> string {
  if(has3EXSignature(rom)) return "3EX";
  if(has3ESignature(rom))  return "3E";
  if(has3FSignature(rom))  return "Enhanced3F";
                           return "Linear";
}

auto Atari2600::hasCommavidSignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x9d, 0xff, 0xf3 },  //STA $F3FF,X
    { 0x99, 0x00, 0xf4 },  //STA $F400,Y
  });
}

auto Atari2600::hasAtariF8Signature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x8d, 0xf9, 0x1f },  //STA $1FF9
    { 0x8d, 0xf9, 0xff },  //STA $FFF9
  }, 2);
}

auto Atari2600::hasActivisionFESignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x20, 0x00, 0xd0, 0xc6, 0xc5 },  //JSR $D000; DEC $C5
    { 0x20, 0xc3, 0xf8, 0xa5, 0x82 },  //JSR $F8C3; LDA $82
    { 0xd0, 0xfb, 0x20, 0x73, 0xfe },  //BNE rel(-5); JSR $FE73
    { 0xd0, 0xfb, 0x20, 0x68, 0xfe },  //BNE rel(-5); JSR $FE68
    { 0x20, 0x00, 0xf0, 0x84, 0xd6 },  //JSR $F000; STY $D6
  });
}

auto Atari2600::hasParkerBrosE0Signature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x8d, 0xe0, 0x1f },  //STA $1FE0
    { 0x8d, 0xe0, 0x5f },  //STA $5FE0
    { 0x8d, 0xe9, 0xff },  //STA $FFE9
    { 0x0c, 0xe0, 0x1f },  //NOP $1FE0
    { 0xad, 0xe0, 0x1f },  //LDA $1FE0
    { 0xad, 0xe9, 0xff },  //LDA $FFE9
    { 0xad, 0xed, 0xff },  //LDA $FFED
    { 0xad, 0xf3, 0xbf },  //LDA $BFF3
  });
}

auto Atari2600::has3EPlusSignature(std::vector<u8>& rom) -> bool {
  return match(rom, { 'T', 'J', '3', 'E' });
}

auto Atari2600::has3EXSignature(std::vector<u8>& rom) -> bool {
  return match(rom, { '3', 'E', 'X' }, 2);
}

auto Atari2600::has3ESignature(std::vector<u8>& rom) -> bool {
  if(has3EPlusSignature(rom) || has3EXSignature(rom)) return false;
  return match(rom, { 0x85, 0x3e })      //STA $3E
      && match(rom, { 0x85, 0x3f }, 2);  //STA $3F
}

auto Atari2600::has3FSignature(std::vector<u8>& rom) -> bool {
  if(has3EPlusSignature(rom) || has3EXSignature(rom)) return false;
  return match(rom, { 0x85, 0x3f }, 2);  //STA $3F
}

auto Atari2600::hasMNetwork8KiBSignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0xad, 0xe4, 0xff },  //LDA $FFE4
    { 0xad, 0xe5, 0xff },  //LDA $FFE5
    { 0xad, 0xe6, 0xff },  //LDA $FFE6
  });
}

auto Atari2600::hasMNetworkSignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0xad, 0xe2, 0xff },  //LDA $FFE2
    { 0xad, 0xe5, 0xff },  //LDA $FFE5
    { 0xad, 0xe5, 0x1f },  //LDA $1FE5
    { 0xad, 0xe7, 0x1f },  //LDA $1FE7
    { 0x0c, 0xe7, 0x1f },  //NOP $1FE7
    { 0x8d, 0xe7, 0xff },  //STA $FFE7
    { 0x8d, 0xe7, 0x1f },  //STA $1FE7
  });
}

auto Atari2600::hasUASignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x8d, 0x40, 0x02 },  //STA $0240
    { 0xad, 0x40, 0x02 },  //LDA $0240
    { 0xbd, 0x1f, 0x02 },  //LDA $021F,X
    { 0x2c, 0xc0, 0x02 },  //BIT $02C0
    { 0x8d, 0xc0, 0x02 },  //STA $02C0
    { 0xad, 0xc0, 0x02 },  //LDA $02C0
    { 0x2c, 0xb0, 0x0f },  //BIT $0FB0
  });
}

auto Atari2600::hasJVPSignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x2c, 0xc0, 0x0f },  //BIT $0FC0
    { 0x8d, 0xc0, 0x0f },  //STA $0FC0
    { 0xad, 0xc0, 0x0f },  //LDA $0FC0
    { 0x2c, 0xc0, 0xef },  //BIT $EFC0
  });
}

auto Atari2600::hasEconoBanking0840Signature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0xad, 0x00, 0x08 },        //LDA $0800
    { 0xad, 0x40, 0x08 },        //LDA $0840
    { 0x2c, 0x00, 0x08 },        //BIT $0800
    { 0x0c, 0x00, 0x08, 0x4c },  //NOP $0800; JMP ...
    { 0x0c, 0xff, 0x0f, 0x4c },  //NOP $0FFF; JMP ...
  }, 2);
}

auto Atari2600::hasWicksteadSignature(std::vector<u8>& rom) -> bool {
  return match(rom, { 0xa5, 0x39, 0x4c });  //LDA $39; JMP ...
}

auto Atari2600::hasJaneSignature(std::vector<u8>& rom) -> bool {
  return match(rom, { 0xad, 0xf1, 0xff, 0x60 });  //LDA $FFF1; RTS
}

auto Atari2600::hasParkerBros03E0Signature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x0d, 0xe0, 0x03, 0x0d },  //ORA $03E0; ORA ...
    { 0xad, 0xe0, 0x03, 0xad },  //LDA $03E0; LDA ...
  });
}

auto Atari2600::hasCPUWiz4KSCSignature(std::vector<u8>& rom) -> bool {
  if(rom.size() != 4_KiB) return false;
  if(std::equal(rom.begin(), rom.begin() + 2_KiB, rom.begin() + 2_KiB)) return false;
  for(u32 offset : range(1, 256)) {
    if(rom[offset] != rom[0]) return false;
  }
  return rom[0x0ffa] == 'S' && rom[0x0ffb] == 'C';
}

auto Atari2600::hasDPCPlusSignature(std::vector<u8>& rom) -> bool {
  return match(rom, { 'D', 'P', 'C', '+' }, 2);
}

auto Atari2600::hasAmigaFCSignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0x8d, 0xf8, 0x1f, 0x4a, 0x4a, 0x8d },  //STA $1FF8; LSR A; LSR A; STA ...
    { 0x8d, 0xf8, 0xff, 0x8d, 0xfc, 0xff },  //STA $FFF8; STA $FFFC
    { 0x8c, 0xf9, 0xff, 0xad, 0xfc, 0xff },  //STY $FFF9; LDA $FFFC
  });
}

auto Atari2600::has4A50Signature(std::vector<u8>& rom) -> bool {
  if(rom.size() != 64_KiB && rom.size() != 128_KiB) return false;
  if(rom[rom.size() - 6] == 0x50 && rom[rom.size() - 5] == 0x4a) return true;

  auto target = rom[0xfffc] | rom[0xfffd] << 8;
  return (rom[0xfffd] & 0x1f) == 0x1f && target + 2 < rom.size()
      && rom[target] == 0x0c && (rom[target + 2] & 0xfe) == 0x6e;
}

auto Atari2600::hasEFSCSignature(std::vector<u8>& rom) -> bool {
  if(hasTailMarker(rom, {'E', 'F', 'S', 'C'})) return true;
  return hasRepeatedRamWindow(rom) && matchAny(rom, {
    { 0x0c, 0xe0, 0xff },  //NOP $FFE0
    { 0xad, 0xe0, 0xff },  //LDA $FFE0
    { 0x0c, 0xe0, 0x1f },  //NOP $1FE0
    { 0xad, 0xe0, 0x1f },  //LDA $1FE0
  });
}

auto Atari2600::hasEFSignature(std::vector<u8>& rom) -> bool {
  if(hasTailMarker(rom, {'E', 'F', 'S', 'C'})) return false;
  if(hasTailMarker(rom, {'E', 'F', 'F', 'B'})) return false;
  if(hasTailMarker(rom, {'E', 'F', 'E', 'F'})) return true;
  if(hasRepeatedRamWindow(rom)) return false;
  return matchAny(rom, {
    { 0x0c, 0xe0, 0xff },  //NOP $FFE0
    { 0xad, 0xe0, 0xff },  //LDA $FFE0
    { 0x0c, 0xe0, 0x1f },  //NOP $1FE0
    { 0xad, 0xe0, 0x1f },  //LDA $1FE0
  });
}

auto Atari2600::hasDFSignature(std::vector<u8>& rom) -> bool {
  if(hasTailMarker(rom, {'D', 'F', 'S', 'C'})) return false;
  if(hasTailMarker(rom, {'D', 'F', 'D', 'F'})) return true;
  return false;
}

auto Atari2600::hasDFSCSignature(std::vector<u8>& rom) -> bool {
  return hasTailMarker(rom, {'D', 'F', 'S', 'C'});
}

auto Atari2600::hasBFSignature(std::vector<u8>& rom) -> bool {
  if(hasTailMarker(rom, {'B', 'F', 'S', 'C'})) return false;
  if(hasTailMarker(rom, {'B', 'F', 'B', 'F'})) return true;
  return false;
}

auto Atari2600::hasBFSCSignature(std::vector<u8>& rom) -> bool {
  return hasTailMarker(rom, {'B', 'F', 'S', 'C'});
}

auto Atari2600::hasMDMSignature(std::vector<u8>& rom) -> bool {
  static const std::vector<u8> marker{'M', 'D', 'M', 'C'};
  auto end = rom.begin() + std::min<u32>(rom.size(), 8_KiB);
  return std::search(rom.begin(), end, marker.begin(), marker.end()) != end;
}

auto Atari2600::hasX07Signature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0xad, 0x0d, 0x08 },  //LDA $080D
    { 0xad, 0x1d, 0x08 },  //LDA $081D
    { 0xad, 0x2d, 0x08 },  //LDA $082D
    { 0x0c, 0x0d, 0x08 },  //NOP $080D
    { 0x0c, 0x1d, 0x08 },  //NOP $081D
    { 0x0c, 0x2d, 0x08 },  //NOP $082D
  });
}

auto Atari2600::hasSuperbankingSignature(std::vector<u8>& rom) -> bool {
  return matchAny(rom, {
    { 0xbd, 0x00, 0x08 },  //LDA $0800,X
    { 0xad, 0x00, 0x08 },  //LDA $0800
  });
}

auto Atari2600::matchAny(std::vector<u8>& rom, std::initializer_list<std::vector<u8>> patterns,
  u8 targetMatches) -> bool {
  for(auto& pattern : patterns) {
    if(match(rom, pattern, targetMatches)) return true;
  }
  return false;
}

auto Atari2600::hasTailMarker(std::vector<u8>& rom, std::vector<u8> marker) -> bool {
  if(rom.size() < 8 || marker.empty() || marker.size() > 8) return false;
  for(u32 offset = rom.size() - 8; offset + marker.size() <= rom.size(); offset++) {
    if(std::equal(marker.begin(), marker.end(), rom.begin() + offset)) return true;
  }
  return false;
}

auto Atari2600::hasRepeatedRamWindow(std::vector<u8>& rom) -> bool {
  if(rom.empty() || rom.size() % 4_KiB) return false;
  for(u32 bank = 0; bank < rom.size(); bank += 4_KiB) {
    for(u32 index = 0; index < 128; index++) {
      if(rom[bank + index] != rom[bank + 128 + index]) return false;
    }
  }
  return true;
}

auto Atari2600::hasSaraRamLayout(std::vector<u8>& rom) -> bool {
  if(rom.size() != 8_KiB && rom.size() != 16_KiB && rom.size() != 32_KiB) return false;
  return hasRepeatedRamWindow(rom);
}

auto Atari2600::normalizeWDSW(std::vector<u8>& rom) -> void {
  if(rom.size() != 8195 || !match(rom, { 0xa5, 0x39, 0x4c })) return;
  rom.resize(8_KiB);
  std::swap_ranges(rom.begin() + 0x0800, rom.begin() + 0x0c00, rom.begin() + 0x0c00);
}

auto Atari2600::match(std::vector<u8>& rom, std::vector<u8> pattern, u8 target_matches) -> bool {
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
