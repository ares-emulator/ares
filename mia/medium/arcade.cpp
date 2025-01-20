struct Arcade : Mame {
  auto name() -> string override { return "Arcade"; }
  auto extensions() -> vector<string> override { return {}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Arcade::load(string location) -> LoadResult {
  auto foundDatabase = Medium::loadDatabase();
  if(!foundDatabase) return LoadResult(databaseNotFound);
  manifest = manifestDatabaseArcade(Medium::name(location));
  if(!manifest) return LoadResult(romNotFoundInDatabase);

  auto document = BML::unserialize(manifest);
  if(!document) return LoadResult(couldNotParseManifest);

  //Sega SG-1000 based arcade
  if(document["game/board"].string() == "sega/sg1000a") {
    vector<u8> rom = loadRoms(location, document, "maincpu");
    if(!rom) return romNotFound;

    this->location = location;

    pak = new vfs::directory;
    pak->setAttribute("board",  document["game/board" ].string());
    pak->setAttribute("title",  document["game/title"].string());
    pak->setAttribute("region", document["game/region"].string());
    pak->append("manifest.bml", manifest);
    pak->append("program.rom",  rom);

    return LoadResult(successful);
  }

  return LoadResult(otherError);
}

auto Arcade::save(string location) -> bool {
  return true;
}
