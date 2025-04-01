struct MegaCD : CompactDisc {
  auto name() -> string override { return "Mega CD"; }
  auto extensions() -> vector<string> override {
#if defined(ARES_ENABLE_CHD)
    return {"cue", "chd"};
#else
    return {"cue"};
#endif
  }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
};

auto MegaCD::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("serial", document["game/serial"].string());
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

auto MegaCD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto MegaCD::analyze(string location) -> string {
  vector<u8> sector;

  sector = readDataSector(location, 0);

  if(!sector || memory::compare(sector.data(), "SEGA", 4))
    return CompactDisc::manifestAudio(location);

  vector<string> regions;
  if(!memory::compare(sector.data()+4, "DISCSYSTEM  ", 12)
  || !memory::compare(sector.data()+4, "BOOTDISC    ", 12)) {
    if(     Hash::CRC32({sector.data()+0x200,  340}).value() == 0x4571f623) // JP boot
      regions.append("NTSC-J");
    else if(Hash::CRC32({sector.data()+0x200, 1390}).value() == 0x6ffb4732) // EU boot
      regions.append("PAL");
    else if(Hash::CRC32({sector.data()+0x200, 1412}).value() == 0xf361ab57) // US boot
      regions.append("NTSC-U");
  }
  if(!regions) regions.append("NTSC-J","NTSC-U","PAL"); // unknown boot

  string serialNumber = slice((const char*)(sector.data() + 0x180), 0, 14).trimRight(" ");

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

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  serial: ", serialNumber, "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  if(devices)
  s +={"  device: ", devices.merge(", "), "\n"};
  return s;
}
