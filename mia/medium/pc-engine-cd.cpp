struct PCEngineCD : CompactDisc {
  auto name() -> string override { return "PC Engine CD"; }
  auto extensions() -> vector<string> override { return {"pcecd", "cue"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
};

auto PCEngineCD::load(string location) -> bool {
  if(!inode::exists(location)) return false;

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("cd.rom", vfs::disk::open({location, "cd.rom"}, vfs::read));
  }
  if(file::exists(location)) {
    pak->append("cd.rom", vfs::cdrom::open(location));
  }

  return true;
}

auto PCEngineCD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto PCEngineCD::analyze(string location) -> string {
  auto sector = readDataSectorCUE(location, 0);
  if(!sector) return CompactDisc::manifestAudio(location);

  //yes, "Electronics" is spelled incorrectly in actual PC Engine CD games ...
  if(memory::compare(sector.data() + 0x264, "NEC Home Electoronics", 21)) {
    return CompactDisc::manifestAudio(location);
  }

  //note: there is no method to determine the region for PC Engine CDs ...
  string region = "NTSC-J";

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  return s;
}
