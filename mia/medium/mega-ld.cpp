struct MegaLD : LaserDisc {
  auto name() -> string override { return "Mega LD"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(string digitalTrack) -> string;
};

auto MegaLD::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  if(!location.iendsWith(".mmi")) return invalidROM;
  if(!mmiArchive.open(location)) return invalidROM;
  if(!mmiArchive.media().size()) return invalidROM;

  if(mmiArchive.media().size() > 1) {
    //TODO: Multi-side media is not yet supported
    return invalidROM;
  }

  // Mega-LD games must have a digital track
  string digitalTrack;
  for(auto& stream : mmiArchive.media().first().streams) {
    if(stream.name == "DigitalAudio") digitalTrack = stream.file;
  }
  if(!digitalTrack) return invalidROM;

  this->location = location;
  this->manifest = analyze(digitalTrack);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("location", location);
  pak->setAttribute("system", mmiArchive.system());
  pak->append("manifest.bml", manifest);
  if(file::exists(location)) {
    pak->append("cd.rom", vfs::cdrom::open(location, digitalTrack));
  }

  return successful;
}

auto MegaLD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto MegaLD::analyze(string digitalTrack) -> string {
  auto sector = readDataSector(location, digitalTrack, 0);
  if(!sector || memory::compare(sector.data(), "SEGA", 4)) return {};

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
