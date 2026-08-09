struct Atari5200 : Cartridge {
  auto name() -> string override { return "Atari 5200"; }
  auto extensions() -> std::vector<string> override { return {"a52", "bin", "car"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;

private:
  struct Board {
    string name;
    u32 size = 0;

    explicit operator bool() const { return size; }
  };

  enum class CARTKind {
    Unknown,
    AtariComputer,
    Atari5200,
    UnsupportedAtari5200,
  };

  struct CARTLayout {
    CARTKind kind = CARTKind::Unknown;
    Board board;
  };

  auto loadDirectory(string location) -> LoadResult;
  auto loadCART(string location, const std::vector<u8>& image) -> LoadResult;
  auto loadROM(string location, std::vector<u8> rom, Board expectedBoard) -> LoadResult;
  auto mount(string location, std::vector<u8> rom, string packageManifest) -> LoadResult;

  auto validateManifest(const string& packageManifest, const std::vector<u8>& rom) const -> string;
  auto analyze(string location, const std::vector<u8>& rom, const Board& board, string digest) const -> string;

  auto identifyBoard(string name) const -> Board;
  auto identifyCART(u32 type) const -> CARTLayout;
  auto identifySource(string digest) const -> Board;
  auto inferBoard(u32 size) const -> Board;
  auto normalize(Board& board, std::vector<u8>& rom) const -> void;
  auto checksum(const std::vector<u8>& image, u32 offset) const -> u32;
};

auto Atari5200::load(string location) -> LoadResult {
  this->location = {};
  pak.reset();
  manifest = {};
  sha256 = {};

  if(directory::exists(location)) return loadDirectory(location);
  if(!file::exists(location)) return romNotFound;

  auto image = Cartridge::read(location);
  if(image.empty()) return romNotFound;

  bool hasCARTMagic = image.size() >= 4
    && image[0] == 'C' && image[1] == 'A' && image[2] == 'R' && image[3] == 'T';
  if(hasCARTMagic || location.iendsWith(".car")) return loadCART(location, image);
  return loadROM(location, std::move(image), {});
}

auto Atari5200::loadDirectory(string location) -> LoadResult {
  auto manifestPath = string{location, "/manifest.bml"};
  auto programPath = string{location, "/program.rom"};
  if(!file::exists(manifestPath) || !file::exists(programPath)) {
    return {invalidROM, "The normalized package requires manifest.bml and program.rom. "};
  }

  string packageManifest = file::read(manifestPath);
  auto rom = file::read(programPath);
  if(rom.empty()) return {invalidROM, "The normalized package has an empty program.rom. "};

  auto detail = validateManifest(packageManifest, rom);
  if(detail) return {invalidROM, detail};
  return mount(location, std::move(rom), std::move(packageManifest));
}

auto Atari5200::loadCART(string location, const std::vector<u8>& image) -> LoadResult {
  if(image.size() < 16) return {invalidROM, "The CART header is truncated. "};
  if(image[0] != 'C' || image[1] != 'A' || image[2] != 'R' || image[3] != 'T') {
    return {invalidROM, "The CART header magic is invalid. "};
  }

  u32 type = (u32)image[4] << 24 | (u32)image[5] << 16 | (u32)image[6] << 8 | image[7];
  auto layout = identifyCART(type);

  switch(layout.kind) {
  case CARTKind::AtariComputer: {
    LoadResult result(wrongMediaType);
    result.mediaType = "Atari 8-bit computer cartridge";
    result.info = {"CART type ", type, ". "};
    return result;
  }
  case CARTKind::UnsupportedAtari5200:
    return {invalidROM, {"CART type ", type, " is an unsupported Atari 5200 cartridge layout. "}};
  case CARTKind::Unknown:
    return {invalidROM, {"CART type ", type, " is not recognized. "}};
  case CARTKind::Atari5200:
    break;
  }

  if(image.size() != 16 + layout.board.size) {
    return {invalidROM, {"CART type ", type, " requires exactly ", layout.board.size, " payload bytes. "}};
  }
  if(image[12] || image[13] || image[14] || image[15]) {
    return {invalidROM, "The CART reserved header bytes must be zero. "};
  }

  u32 expectedChecksum = (u32)image[8] << 24 | (u32)image[9] << 16 | (u32)image[10] << 8 | image[11];
  if(checksum(image, 16) != expectedChecksum) return {invalidROM, "The CART payload checksum is invalid. "};

  std::vector<u8> rom(image.begin() + 16, image.end());
  normalize(layout.board, rom);

  return loadROM(location, std::move(rom), layout.board);
}

auto Atari5200::loadROM(string location, std::vector<u8> rom, Board expectedBoard) -> LoadResult {
  auto digest = Hash::SHA256(rom).digest();
  if(!expectedBoard) {
    expectedBoard = identifySource(digest);
    if(expectedBoard) {
      normalize(expectedBoard, rom);
      digest = Hash::SHA256(rom).digest();
    }
  }

  auto databaseManifest = manifestDatabase(digest);
  if(databaseManifest) {
    auto detail = validateManifest(databaseManifest, rom);
    if(detail) return {couldNotParseManifest, {"Atari 5200.bml: ", detail}};

    if(expectedBoard) {
      auto document = BML::unserialize(databaseManifest);
      if(document["game/board"].string() != expectedBoard.name) {
        return {couldNotParseManifest, "Atari 5200.bml: CART type and database board disagree. "};
      }
    }
    return mount(location, std::move(rom), std::move(databaseManifest));
  }

  if(!expectedBoard) expectedBoard = inferBoard(rom.size());
  if(!expectedBoard && rom.size() == 16_KiB) {
    return {
      invalidROM,
      "Raw 16 KiB images require an Atari 5200 database match to select one- or two-chip wiring. "
    };
  }
  if(!expectedBoard && rom.size() == 40_KiB) {
    return {
      invalidROM,
      "Raw 40 KiB images require an Atari 5200 database match to select the Bounty Bob source variant. "
    };
  }
  if(!expectedBoard) return {invalidROM, {"Raw cartridge size ", rom.size(), " is not supported. "}};

  auto fallbackManifest = analyze(location, rom, expectedBoard, digest);
  return mount(location, std::move(rom), std::move(fallbackManifest));
}

auto Atari5200::mount(string location, std::vector<u8> rom, string packageManifest) -> LoadResult {
  auto document = BML::unserialize(packageManifest);
  if(!document) return couldNotParseManifest;

  this->location = location;
  this->manifest = packageManifest;
  sha256 = Hash::SHA256(rom).digest();
  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board", document["game/board"].string());
  pak->append("manifest.bml", packageManifest);
  pak->append("program.rom", rom);
  return successful;
}

auto Atari5200::validateManifest(const string& packageManifest, const std::vector<u8>& rom) const -> string {
  auto document = BML::unserialize(packageManifest);
  if(!document) return "The manifest could not be parsed. ";

  auto board = identifyBoard(document["game/board"].string());
  if(!board) return {"The manifest board '", document["game/board"].string(), "' is unsupported. "};

  auto memories = document.find("game/board/memory(type=ROM,content=Program)");
  if(memories.size() != 1) return "The manifest must contain exactly one Program ROM memory node. ";
  if(memories[0]["size"].natural() != board.size) {
    return {"The manifest Program ROM size does not match board ", board.name, ". "};
  }
  if(rom.size() != board.size) return {"program.rom does not match board ", board.name, ". "};

  auto expectedHash = document["game/sha256"].string();
  if(expectedHash && expectedHash != Hash::SHA256(rom).digest()) {
    return "program.rom does not match the manifest SHA-256. ";
  }
  return {};
}

auto Atari5200::analyze(string location, const std::vector<u8>& rom, const Board& board,
  string digest) const -> string {
  string manifest;
  manifest += "game\n";
  manifest +={"  name:   ", Medium::name(location), "\n"};
  manifest +={"  title:  ", Medium::name(location), "\n"};
  manifest += "  region: NTSC\n";
  manifest +={"  sha256: ", digest, "\n"};
  manifest +={"  board:  ", board.name, "\n"};
  manifest += "    memory\n";
  manifest += "      type: ROM\n";
  manifest +={"      size: 0x", hex(rom.size()), "\n"};
  manifest += "      content: Program\n";
  return manifest;
}

auto Atari5200::identifyBoard(string name) const -> Board {
  if(name == "Linear4K"      ) return {name,  4_KiB};
  if(name == "Linear8K"      ) return {name,  8_KiB};
  if(name == "OneChip16K"    ) return {name, 16_KiB};
  if(name == "TwoChip16K"    ) return {name, 16_KiB};
  if(name == "Overlapping16K") return {name, 16_KiB};
  if(name == "Linear32K"     ) return {name, 32_KiB};
  if(name == "DualWindow40K" ) return {name, 40_KiB};
  return {};
}

auto Atari5200::identifyCART(u32 type) const -> CARTLayout {
  if(type ==   4) return {CARTKind::Atari5200, identifyBoard("Linear32K")};
  if(type ==   6) return {CARTKind::Atari5200, identifyBoard("TwoChip16K")};
  if(type ==   7) return {CARTKind::Atari5200, identifyBoard("DualWindow40K")};
  if(type ==  16) return {CARTKind::Atari5200, identifyBoard("OneChip16K")};
  if(type ==  19) return {CARTKind::Atari5200, identifyBoard("Linear8K")};
  if(type ==  20) return {CARTKind::Atari5200, identifyBoard("Linear4K")};
  if(type == 159) return {CARTKind::Atari5200, {"DualWindow40KAlt", 40_KiB}};
  if(type >= 71 && type <= 74) return {CARTKind::UnsupportedAtari5200};
  if((type >= 1 && type <= 112) || type == 160) return {CARTKind::AtariComputer};
  return {};
}

auto Atari5200::identifySource(string digest) const -> Board {
  if(digest == "957aee5457f89ff9a89b87d045f4803c870b54f40045e3c397b47f716fedadff") {
    return identifyCART(159).board;
  }
  return {};
}

auto Atari5200::inferBoard(u32 size) const -> Board {
  if(size ==  4_KiB) return identifyBoard("Linear4K");
  if(size ==  8_KiB) return identifyBoard("Linear8K");
  if(size == 32_KiB) return identifyBoard("Linear32K");
  return {};
}

auto Atari5200::normalize(Board& board, std::vector<u8>& rom) const -> void {
  if(board.name != "DualWindow40KAlt") return;
  std::rotate(rom.begin(), rom.begin() + 0x2000, rom.end());
  board = identifyBoard("DualWindow40K");
}

auto Atari5200::checksum(const std::vector<u8>& image, u32 offset) const -> u32 {
  u32 checksum = 0;
  while(offset < image.size()) checksum += image[offset++];
  return checksum;
}

auto Atari5200::save(string location) -> bool {
  return true;
}
