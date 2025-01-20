struct PCEngineCD : CompactDisc {
  auto name() -> string override { return "PC Engine CD"; }
  auto extensions() -> vector<string> override { return {"cue", "chd"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
};

auto PCEngineCD::load(string location) -> LoadResult {
  if(!inode::exists(location)) return LoadResult(romNotFound);

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return LoadResult(couldNotParseManifest);

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("card",   document["game/card"].string());
  pak->setAttribute("audio",  (bool)document["game/audio"]);
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("cd.rom", vfs::disk::open({location, "cd.rom"}, vfs::read));
  }
  if(file::exists(location)) {
    pak->append("cd.rom", vfs::cdrom::open(location));
  }

  return LoadResult(successful);
}

auto PCEngineCD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto PCEngineCD::analyze(string location) -> string {
  vector<u8> sectors[2];

  if(location.iendsWith(".cue")) {
    sectors[0] = readDataSectorCUE(location, 0);  // NEC
    sectors[1] = readDataSectorCUE(location, 16); // Games Express
  } else if (location.iendsWith(".chd")) {
    sectors[0] = readDataSectorCHD(location, 0);  // NEC
    sectors[1] = readDataSectorCHD(location, 16); // Games Express
  }

  if(!sectors[0] && !sectors[1]) return CompactDisc::manifestAudio(location);

  bool isNEC = sectors[0] && (memory::compare(sectors[0].data() + 0x264, "NEC Home Electoronics", 21) == 0);
  bool isGamesExpress = sectors[1] &&(memory::compare(sectors[1].data() + 0x1, "CD001", 5) == 0);

  if(!isGamesExpress && !isNEC) return CompactDisc::manifestAudio(location);

  //note: there is no method to determine the region for PC Engine CDs from the data itself
  string region = "NTSC-J";
  if(location.ifind("(USA)")) region = "NTSC-U";

  string card = "any";

  //Altered Beast requires Card 1.0; Game doesn't specify its name in the header
  //do our best to guess based on filename
  if(location.ifind("Juuouki")) card = "System Card 1.0";
  if(location.ifind("Juoki")) card = "System Card 1.0";
  if(location.ifind("Altered Beast")) card = "System Card 1.0";

  if(!isNEC && isGamesExpress) card = "Games Express";

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  s +={"  card:   ", card, "\n"};
  return s;
}
