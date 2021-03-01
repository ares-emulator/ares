auto PCEngineCD::load(string location) -> shared_pointer<vfs::directory> {
  if(directory::exists(location)) {
    auto pak = shared_pointer{new vfs::directory};
    pak->append("manifest.bml", manifest(location));
    pak->append("cd.rom", vfs::disk::open({location, "cd.rom"}, vfs::read));
    return pak;
  }

  if(file::exists(location)) {
    auto pak = shared_pointer{new vfs::directory};
    pak->append("manifest.bml", manifest(location));
    pak->append("cd.rom", vfs::cdrom::open(location));
    return pak;
  }

  return {};
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
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  return s;
}
