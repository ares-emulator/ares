auto PCEngineCD::load(string location) -> shared_pointer<vfs::directory> {
  auto pak = shared_pointer{new vfs::directory};
  auto manifest = PCEngineCD::manifest(location);
  auto document = BML::unserialize(manifest);
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("cd.rom", vfs::disk::open({location, "cd.rom"}, vfs::read));
  }
  if(file::exists(location)) {
    pak->append("cd.rom", vfs::cdrom::open(location));
  }
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());

  return pak;
}

auto PCEngineCD::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  return true;
}

auto PCEngineCD::manifest(string location) -> string {
  auto sector = readDataSectorCUE(location, 0);
  if(!sector) return CompactDisc::manifest(location);

  //yes, "Electronics" is spelled incorrectly in actual PC Engine CD games ...
  if(memory::compare(sector.data() + 0x264, "NEC Home Electoronics", 21)) {
    return CompactDisc::manifest(location);
  }

  //note: there is no method to determine the region for PC Engine CDs ...
  string region = "NTSC-J";

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  title:  ", Media::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  return s;
}
