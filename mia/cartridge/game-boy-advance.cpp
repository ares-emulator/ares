struct GameBoyAdvance : Cartridge {
  auto name() -> string override { return "Game Boy Advance"; }
  auto extensions() -> vector<string> override { return {"gba"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto GameBoyAdvance::load(string location) -> shared_pointer<vfs::directory> {
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

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Media::load(location, pak, node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Media::load(location, pak, node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=Flash,content=Save)"]) {
    Media::load(location, pak, node, ".flash");
    if(auto fp = pak->read("save.flash")) {
      fp->setAttribute("manufacturer", node["manufacturer"].string());
    }
  }

  return pak;
}

auto GameBoyAdvance::save(string location, shared_pointer<vfs::directory> pak) -> bool {
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
  if(auto node = document["game/board/memory(type=Flash,content=Save)"]) {
    Media::save(location, pak, node, ".flash");
  }

  return true;
}

auto GameBoyAdvance::heuristics(vector<u8>& data, string location) -> string {
  vector<string> identifiers = {
    "SRAM_V",
    "SRAM_F_V",
    "EEPROM_V",
    "FLASH_V",
    "FLASH512_V",
    "FLASH1M_V",
  };

  vector<string> list;
  for(auto& identifier : identifiers) {
    for(s32 n : range(data.size() - 16)) {
      if(!memory::compare(&data[n], identifier.data(), identifier.size())) {
        auto p = (const char*)&data[n + identifier.size()];
        if(p[0] >= '0' && p[0] <= '9'
        && p[1] >= '0' && p[1] <= '9'
        && p[2] >= '0' && p[2] <= '9'
        ) {
          char text[16];
          memory::copy(text, &data[n], identifier.size() + 3);
          text[identifier.size() + 3] = 0;
          if(!list.find(text)) list.append(text);
        }
      }
    }
  }

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  title: ", Media::name(location), "\n"};
  s += "  board\n";

  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";

  if(list) {
    if(list.first().beginsWith("SRAM_V") || list.first().beginsWith("SRAM_F_V")) {
      s += "    memory\n";
      s += "      type: RAM\n";
      s += "      size: 0x8000\n";
      s += "      content: Save\n";
    }

    if(list.first().beginsWith("EEPROM_V")) {
      s += "    memory\n";
      s += "      type: EEPROM\n";
      s += "      size: 0x0\n";  //auto-detected
      s += "      content: Save\n";
    }

    if(list.first().beginsWith("FLASH_V") || list.first().beginsWith("FLASH512_V")) {
      s += "    memory\n";
      s += "      type: Flash\n";
      s += "      size: 0x10000\n";
      s += "      content: Save\n";
      s += "      manufacturer: Macronix\n";
    }

    if(list.first().beginsWith("FLASH1M_V")) {
      s += "    memory\n";
      s += "      type: Flash\n";
      s += "      size: 0x20000\n";
      s += "      content: Save\n";
      s += "      manufacturer: Macronix\n";
    }
  }

  return s;
}
