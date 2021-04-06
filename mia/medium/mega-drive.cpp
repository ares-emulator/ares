struct MegaDrive : Cartridge {
  auto name() -> string override { return "Mega Drive"; }
  auto extensions() -> vector<string> override { return {"md", "smd", "gen", "bin"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto MegaDrive::load(string location) -> bool {
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
  pak->setAttribute("megacd",   (bool)document["game/device"].string().split(", ").find("Mega CD"));
  pak->append("manifest.bml", manifest);

  //add SVP ROM to image if it is missing
  if(document["game/board/memory(type=ROM,content=SVP)"]) {
    if(memory::compare(rom.data() + rom.size() - 0x800, Resource::MegaDrive::SVP, 0x800)) {
      rom.resize(rom.size() + 0x800);
      memory::copy(rom.data() + rom.size() - 0x800, Resource::MegaDrive::SVP, 0x800);
    }
  }

  array_view<u8> view{rom};
  for(auto node : document.find("game/board/memory(type=ROM)")) {
    string name = {node["content"].string().downcase(), ".rom"};
    u32 size = node["size"].natural();
    if(view.size() < size) break;  //missing firmware
    pak->append(name, {view.data(), size});
    view += size;
  }

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
    if(auto fp = pak->read("save.ram")) {
      fp->setAttribute("mode",   node["mode"].string());
      fp->setAttribute("offset", node["offset"].natural());
    }
  }

  return true;
}

auto MegaDrive::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }

  return true;
}

auto MegaDrive::analyze(vector<u8>& data) -> string {
  if(data.size() < 0x800) return {};

  string ramMode = "none";

  u32 ramFrom = 0;
  ramFrom |= data[0x01b4] << 24;
  ramFrom |= data[0x01b5] << 16;
  ramFrom |= data[0x01b6] <<  8;
  ramFrom |= data[0x01b7] <<  0;

  u32 ramTo = 0;
  ramTo |= data[0x01b8] << 24;
  ramTo |= data[0x01b9] << 16;
  ramTo |= data[0x01ba] <<  8;
  ramTo |= data[0x01bb] <<  0;

  if(!(ramFrom & 1) && !(ramTo & 1)) ramMode = "hi";
  if( (ramFrom & 1) &&  (ramTo & 1)) ramMode = "lo";
  if(!(ramFrom & 1) &&  (ramTo & 1)) ramMode = "word";
  if(data[0x01b0] != 'R' || data[0x01b1] != 'A') ramMode = "none";

  u32 ramSize = ramTo - ramFrom + 1;
  if(ramMode == "hi") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "lo") ramSize = (ramTo >> 1) - (ramFrom >> 1) + 1;
  if(ramMode == "word") ramSize = ramTo - ramFrom + 1;
  if(ramMode != "none") ramSize = bit::round(min(0x20000, ramSize));
  if(ramMode == "none") ramSize = 0;

  vector<string> devices;
  string device = slice((const char*)&data[0x1a0], 0, 16).trimRight(" ");
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
  string region = slice((const char*)&data[0x01f0], 0, 16).trimRight(" ");
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
  memory::copy(domesticName.get(), &data[0x0120], domesticName.size());
  for(auto& c : domesticName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(domesticName.find("  ")) domesticName.replace("  ", " ");
  domesticName.strip();

  string internationalName;
  internationalName.resize(48);
  memory::copy(internationalName.get(), &data[0x0150], internationalName.size());
  for(auto& c : internationalName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(internationalName.find("  ")) internationalName.replace("  ", " ");
  internationalName.strip();

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  label:  ", domesticName, "\n"};
  s +={"  label:  ", internationalName, "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  if(devices)
  s +={"  device: ", devices.merge(", "), "\n"};
  s += "  board\n";

  if(domesticName == "Game Genie") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(data.size()), "\n"};
    s += "      content: Program\n";
    s += "    slot\n";
    s += "      type: Mega Drive\n";
  } else if(domesticName == "SONIC & KNUCKLES") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x200000\n";
    s += "      content: Program\n";
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x40000\n";
    s += "      content: Patch\n";
    s += "    slot\n";
    s += "      type: Mega Drive\n";
  } else if(internationalName == "Virtua Racing") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(data.size()), "\n"};
    s += "      content: Program\n";
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x800\n";
    s += "      content: SVP\n";
  } else {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(data.size()), "\n"};
    s += "      content: Program\n";
  }

  if(ramSize && ramMode != "none") {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(ramSize), "\n"};
    s += "      content: Save\n";
    s +={"      mode: ", ramMode, "\n"};
    s +={"      offset: 0x", hex(ramFrom), "\n"};
  }

  return s;
}
