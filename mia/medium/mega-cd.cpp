struct MegaCD : CompactDisc {
  auto name() -> string override { return "Mega CD"; }
  auto extensions() -> vector<string> override { return {"mcd", "cue"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
};

auto MegaCD::load(string location) -> bool {
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

auto MegaCD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto MegaCD::analyze(string location) -> string {
  auto sector = readDataSectorCUE(location, 0);
  if(!sector) return CompactDisc::manifestAudio(location);

  if(memory::compare(sector.data(), "SEGADISCSYSTEM  ", 16)) {
    return CompactDisc::manifestAudio(location);
  }

  vector<string> devices;
  string device = slice((const char*)(sector.data() + 0x190), 0, 16).trimRight(" ");
  for(auto& id : device) {
    if(id == '0');  //Master System controller
    if(id == '4');  //multitap
    if(id == '6');  //6-button controller
    if(id == 'A');  //analog joystick
    if(id == 'B');  //trackball
    if(id == 'C');  //CD-ROM drive
    if(id == 'D');  //download?
    if(id == 'F');  //floppy drive
    if(id == 'G');  //light gun
    if(id == 'J');  //3-button controller
    if(id == 'K');  //keyboard
    if(id == 'L');  //Activator
    if(id == 'M');  //mouse
    if(id == 'P');  //printer
    if(id == 'R');  //RS-232 modem
    if(id == 'T');  //tablet
    if(id == 'V');  //paddle
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
  }
  if(!regions && region.size() == 1) {
    maybe<u8> bits;
    u8 field = region[0];
    if(field >= '0' && field <= '9') bits = field - '0';
    if(field >= 'A' && field <= 'F') bits = field - 'A' + 10;
    if(bits && *bits & 1) regions.append("NTSC-J");  //domestic 60hz
    if(bits && *bits & 2);                           //domestic 50hz
    if(bits && *bits & 4) regions.append("NTSC-U");  //overseas 60hz
    if(bits && *bits & 8) regions.append("PAL");     //overseas 50hz
  }
  if(!regions) {
    regions.append("NTSC-J", "NTSC-U", "PAL");
  }

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  if(devices)
  s +={"  device: ", devices.merge(", "), "\n"};
  return s;
}
