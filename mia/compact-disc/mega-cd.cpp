auto MegaCD::pak(string location) -> shared_pointer<vfs::directory> {
  if(auto pak = Media::pak(location)) return pak;
  auto pak = shared_pointer{new vfs::directory};
  pak->append("manifest.bml", manifest(location));
  pak->append("cd.rom", vfs::cdrom::open(location));
  return pak;
}

auto MegaCD::rom(string location) -> vector<u8> {
  vector<u8> data;
  append(data, {location, "cd.rom"});
  return data;
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
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  return s;
}
