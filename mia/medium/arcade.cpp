struct Arcade : Mame {
  auto name() -> string override { return "Arcade"; }
  auto extensions() -> std::vector<string> override { return {}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Arcade::load(string location) -> LoadResult {
  auto foundDatabase = Medium::loadDatabase();
  if(!foundDatabase) return { databaseNotFound, "Arcade.bml" };
  manifest = manifestDatabaseArcade(Medium::name(location));
  if(!manifest) return romNotFoundInDatabase;

  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  //Sega SG-1000 based arcade
  if(document["game/board"].string() == "sega/sg1000a") {
    std::vector<u8> rom = loadRoms(location, document, "maincpu");
    if(rom.empty()) return { invalidROM, "Ensure your ROM is in a MAME-compatible .zip format." };

    this->location = location;

    pak = std::make_shared<vfs::directory>();
    pak->setAttribute("board",  document["game/board" ].string());
    pak->setAttribute("name",   document["game/name" ].string());
    pak->setAttribute("title",  document["game/title"].string());
    pak->setAttribute("region", document["game/region"].string());
    pak->append("manifest.bml", manifest);
    pak->append("program.rom",  rom);

    return successful;
  }

  //Aleck 64
  if(document["game/board"].string() == "nintendo/aleck64") {
    std::vector<u8> rom = loadRoms(location, document, "user2");
    if(rom.empty()) return { invalidROM, "Ensure your ROM is in a MAME-compatible .zip format." };

    //MAME stores roms in Byte-Swapped (v64) format, but we need them in their native Big-Endian (z64)
    endianSwap(rom);

    std::vector<u8> pif = loadRoms(location, document, "user1");
    if(pif.empty()) return {
      invalidROM,
      "Ensure your ROM is in a MAME-compatible .zip format and that the Aleck64 pif ROM is available."
    };

    this->location = location;

    pak = std::make_shared<vfs::directory>();
    pak->setAttribute("board",  document["game/board" ].string());
    pak->setAttribute("name",   document["game/name" ].string());
    pak->setAttribute("title",  document["game/title"].string());
    pak->setAttribute("region", document["game/region"].string());
    pak->setAttribute("cic",    "CIC-NUS-5101");
    pak->append("manifest.bml", manifest);
    pak->append("program.rom",  rom);
    pak->append("pif.aleck64.rom", pif);

    return successful;
  }

  return otherError;
}

auto Arcade::save(string location) -> bool {
  return true;
}
