struct WonderSwan : Cartridge {
  auto name() -> string override { return "WonderSwan"; }
  auto extensions() -> vector<string> override { return {"ws"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto WonderSwan::load(string location) -> shared_pointer<vfs::directory> {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return {};

  auto pak = shared_pointer{new vfs::directory};
  auto manifest = Cartridge::manifest(rom, location);
  auto document = BML::unserialize(manifest);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("orientation", document["game/orientation"].string());

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Media::load(location, pak, node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Media::load(location, pak, node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Media::load(location, pak, node, ".rtc");
  }

  return pak;
}

auto WonderSwan::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Media::save(location, pak, node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Media::save(location, pak, node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Media::save(location, pak, node, ".rtc");
  }

  return true;
}

auto WonderSwan::heuristics(vector<u8>& data, string location) -> string {
  if(data.size() < 0x10000) return {};

  auto metadata = &data[data.size() - 16];

  bool color = metadata[7];

  string ramType;
  u32 ramSize = 0;
  switch(metadata[11]) {
  case 0x01: ramType = "RAM";    ramSize =    8_KiB; break;
  case 0x02: ramType = "RAM";    ramSize =   32_KiB; break;
  case 0x03: ramType = "RAM";    ramSize =  128_KiB; break;
  case 0x04: ramType = "RAM";    ramSize =  256_KiB; break;
  case 0x05: ramType = "RAM";    ramSize =  512_KiB; break;
  case 0x10: ramType = "EEPROM"; ramSize =  128;     break;
  case 0x20: ramType = "EEPROM"; ramSize = 2048;     break;
  case 0x50: ramType = "EEPROM"; ramSize = 1024;     break;
  }

  bool orientation = metadata[12] & 1;  //0 = horizontal; 1 = vertical
  bool hasRTC = metadata[13] & 1;

  string s;
  s += "game\n";
  s +={"  name:        ", Media::name(location), "\n"};
  s +={"  title:       ", Media::name(location), "\n"};
  s +={"  orientation: ", !orientation ? "horizontal" : "vertical", "\n"};
  s += "  board\n";

  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";

  if(ramType && ramSize) {
    s += "    memory\n";
    s +={"      type: ", ramType, "\n"};
    s +={"      size: 0x", hex(ramSize), "\n"};
    s += "      content: Save\n";
  }

  if(hasRTC) {
    s += "    memory\n";
    s += "      type: RTC\n";
    s += "      size: 0x10\n";
    s += "      content: Time\n";
  }

  return s;
}
