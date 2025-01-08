struct Arcade : Mame {
  auto name() -> string override { return "Arcade"; }
  auto extensions() -> vector<string> override { return {}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Arcade::load(string location) -> bool {
  manifest = manifestDatabaseArcade(Medium::name(location));
  if(!manifest) return false;

  auto document = BML::unserialize(manifest);
  if(!document) return false;

  //Sega SG-1000 based arcade
  if(document["game/board"].string() == "sega/sg1000a") {
    vector<u8> rom = loadRoms(location, document, "maincpu");
    if(!rom) return false;

    this->location = location;

    pak = new vfs::directory;
    pak->setAttribute("board",  document["game/board" ].string());
    pak->setAttribute("name",   document["game/name" ].string());
    pak->setAttribute("title",  document["game/title"].string());
    pak->setAttribute("region", document["game/region"].string());
    pak->append("manifest.bml", manifest);
    pak->append("program.rom",  rom);

    return true;
  }

  //Aleck 64
  if(document["game/board"].string() == "nintendo/aleck64") {
    vector<u8> rom = loadRoms(location, document, "user2");
    if(!rom) return false;

    //MAME stores roms in Byte-Swapped (v64) format, but we need them in their native Big-Endian (z64)
    endianSwap(rom);

    vector<u8> pif = loadRoms(location, document, "user1");
    if(!pif) return false;

    this->location = location;

    pak = new vfs::directory;
    pak->setAttribute("board",  document["game/board" ].string());
    pak->setAttribute("name",   document["game/name" ].string());
    pak->setAttribute("title",  document["game/title"].string());
    pak->setAttribute("region", document["game/region"].string());
    pak->setAttribute("cic",    "CIC-NUS-5101");
    pak->append("manifest.bml", manifest);
    pak->append("program.rom",  rom);
    pak->append("pif.rom", pif);

    return true;
  }

  return false;
}

auto Arcade::save(string location) -> bool {
  return true;
}
