struct Saturn : CompactDisc {
  auto name() -> string override { return "Saturn"; }
  auto extensions() -> vector<string> override { return {"cue", "chd"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
};

auto Saturn::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("audio", (bool)document["game/audio"]);
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("cd.rom", vfs::disk::open({location, "cd.rom"}, vfs::read));
  }
  if(file::exists(location)) {
    pak->append("cd.rom", vfs::cdrom::open(location));
  }

  return successful;
}

auto Saturn::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto Saturn::analyze(string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", "NTSC-J", "\n"};
  return s;
}
