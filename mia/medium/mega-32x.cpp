struct Mega32X : Cartridge {
  auto name() -> string override { return "Mega 32X"; }
  auto extensions() -> vector<string> override { return {"32x"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
  auto analyzeStorage(vector<u8>& rom, string hash) -> void;

  struct RAM {
    explicit operator bool() const { return mode && size != 0; }

    string mode;
    u32 size = 0;
  } ram;

  struct EEPROM {
    explicit operator bool() const { return mode && size != 0; }

    string mode;
    u32 size = 0;
    u8 rsda = 0;
    u8 wsda = 0;
    u8 wscl = 0;
  } eeprom;
};

auto Mega32X::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return false;

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title",    document["game/title"].string());
  pak->setAttribute("region",   document["game/region"].string());
  pak->setAttribute("bootable", true);
  pak->setAttribute("mega32x",  true);
  pak->setAttribute("megacd",   (bool)document["game/device"].string().split(", ").find("Mega CD"));
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Pak::load(node, ".ram");
    if(auto fp = pak->read("save.ram")) {
      fp->setAttribute("mode", node["mode"].string());
    }
  }

  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Pak::load(node, ".eeprom");
    if(auto fp = pak->read("save.eeprom")) {
      fp->setAttribute("mode", node["mode"].string());
      fp->setAttribute("rsda", node["rsda"].natural());
      fp->setAttribute("wsda", node["wsda"].natural());
      fp->setAttribute("wscl", node["wscl"].natural());
    }
  }

  return true;
}

auto Mega32X::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Pak::save(node, ".ram");
  }

  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Pak::save(node, ".eeprom");
  }

  return true;
}

auto Mega32X::analyze(vector<u8>& rom) -> string {
  if(rom.size() < 0x800) return {};

  ram = {};
  eeprom = {};

  auto hash = Hash::SHA256(rom).digest();
  analyzeStorage(rom, hash);

  vector<string> devices;
  string device = slice((const char*)&rom[0x190], 0, 16).trimRight(" ");
  for(auto& id : device) {
    if(id == '0');  //Master System controller
    if(id == '4');  //multitap
    if(id == '6');  //6-button controller
    if(id == 'A');  //analog joystick
    if(id == 'B');  //trackball
    if(id == 'C') devices.append("Mega CD");
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
  string region = slice((const char*)&rom[0x01f0], 0, 16).trimRight(" ");
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

  string domesticName;
  domesticName.resize(48);
  memory::copy(domesticName.get(), &rom[0x0120], domesticName.size());
  for(auto& c : domesticName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(domesticName.find("  ")) domesticName.replace("  ", " ");
  domesticName.strip();

  string internationalName;
  internationalName.resize(48);
  memory::copy(internationalName.get(), &rom[0x0150], internationalName.size());
  for(auto& c : internationalName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(internationalName.find("  ")) internationalName.replace("  ", " ");
  internationalName.strip();

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Pak::name(location), "\n"};
  s +={"  title:  ", Pak::name(location), "\n"};
  s +={"  label:  ", domesticName, "\n"};
  s +={"  label:  ", internationalName, "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  if(devices)
  s +={"  device: ", devices.merge(", "), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";

  if(eeprom) {
    s += "    memory\n";
    s += "      type: EEPROM\n";
    s +={"      size: 0x", hex(eeprom.size), "\n"};
    s += "      content: Save\n";
    s +={"      mode: ", eeprom.mode, "\n"};
    s +={"      rsda: ", eeprom.rsda, "\n"};
    s +={"      wsda: ", eeprom.wsda, "\n"};
    s +={"      wscl: ", eeprom.wscl, "\n"};
  } else if(ram) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(ram.size), "\n"};
    s += "      content: Save\n";
    s +={"      mode: ", ram.mode, "\n"};
  }
  return s;
}

auto Mega32X::analyzeStorage(vector<u8>& rom, string hash) -> void {
  //SRAM
  //====

  if(rom[0x01b0] == 'R' && rom[0x01b1] == 'A') {
    u32 ramFrom = 0;
    ramFrom |= rom[0x01b4] << 24;
    ramFrom |= rom[0x01b5] << 16;
    ramFrom |= rom[0x01b6] <<  8;
    ramFrom |= rom[0x01b7] <<  0;

    u32 ramTo = 0;
    ramTo |= rom[0x01b8] << 24;
    ramTo |= rom[0x01b9] << 16;
    ramTo |= rom[0x01ba] <<  8;
    ramTo |= rom[0x01bb] <<  0;

    if(!(ramFrom & 1) && !(ramTo & 1)) ram.mode = "upper";
    if( (ramFrom & 1) &&  (ramTo & 1)) ram.mode = "lower";
    if(!(ramFrom & 1) &&  (ramTo & 1)) ram.mode = "word";

    if(ram.mode == "upper") ram.size = (ramTo - ramFrom + 2) >> 1;
    if(ram.mode == "lower") ram.size = (ramTo - ramFrom + 2) >> 1;
    if(ram.mode == "word" ) ram.size = (ramTo - ramFrom + 1);
  }

  //M24C02
  //======

  //NFL Quarterback Club (World)
  if(hash == "092ad8896ea7784d2feb7f17b1a348562588a361c50de6ade722a5c5cce698d5") {
    eeprom.mode = "M24C02";
    eeprom.size = 256;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }

  //M24C04
  //======

  //NBA Jam: Tournament Edition (World)
  if(hash == "7f7e1059671a99405e3782cd9b4e5d4025ee4bb122a89357b95b0fcac512f74f") {
    eeprom.mode = "M24C04";
    eeprom.size = 512;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }
}
