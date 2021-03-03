auto MegaCD::load(string location) -> shared_pointer<vfs::directory> {
  auto pak = shared_pointer{new vfs::directory};
  auto manifest = MegaCD::manifest(location);
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

auto MegaCD::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  return true;
}

auto MegaCD::manifest(string location) -> string {
  auto sector = readDataSectorCUE(location, 0);
  if(!sector) return CompactDisc::manifest(location);

  if(memory::compare(sector.data(), "SEGADISCSYSTEM  ", 16)) {
    return CompactDisc::manifest(location);
  }

  vector<string> regions;
  string region = slice((const char*)(sector.data() + 0x1f0), 0, 16).trimRight(" ");
  if(!regions) {
    if(region == "JAPAN" ) regions.append("NTSC-J");
    if(region == "EUROPE") regions.append("PAL");
  }
  if(!regions) {
    if(region.find("J")) regions.append("NTSC-J");
    if(region.find("U")) regions.append("NTSC-U");
    if(region.find("E")) regions.append("PAL");
    if(region.find("W")) regions.append("NTSC-J", "NTSC-U", "PAL");
  }
  if(!regions && region.size() == 1) {
    u8 field = region.hex();
    if(field & 0x01) regions.append("NTSC-J");
    if(field & 0x04) regions.append("NTSC-U");
    if(field & 0x08) regions.append("PAL");
  }
  if(!regions) {
    regions.append("NTSC-J");
  }

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  title:  ", Media::name(location), "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  return s;
}
