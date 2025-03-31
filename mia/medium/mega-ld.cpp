struct MegaLD : CompactDisc {
  auto name() -> string override { return "Mega LD"; }
  auto extensions() -> vector<string> override { return {"mcd", "cue"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
};

auto MegaLD::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

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

  return successful;
}

auto MegaLD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto MegaLD::analyze(string location) -> string {
  vector<u8> sector;

  if(location.iendsWith(".cue")) {
      sector = readDataSectorCUE(location, 0);
  } else if (location.iendsWith(".chd")) {
      sector = readDataSectorCHD(location, 0);
  }

  if(!sector || memory::compare(sector.data(), "SEGA", 4))
    return CompactDisc::manifestAudio(location);

  vector<string> regions;
  if(!memory::compare(sector.data()+4, "DISCSYSTEM  ", 12)
  || !memory::compare(sector.data()+4, "BOOTDISC    ", 12)) {
    //##TODO## Same hash here, need to find reliable detection. Discs may all be multi-region anyway though, need to confirm.
    if(Hash::CRC32({sector.data()+0x200,  340}).value() == 0xB53fCE8B) // JP boot
      regions.append("NTSC-J");
    if(Hash::CRC32({sector.data()+0x200,  340}).value() == 0xB53fCE8B) // US boot
      regions.append("NTSC-U");
  }
  if(!regions) regions.append("NTSC-J","NTSC-U"); // unknown boot

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
