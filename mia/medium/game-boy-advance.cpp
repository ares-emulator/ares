struct GameBoyAdvance : Cartridge {
  auto name() -> string override { return "Game Boy Advance"; }
  auto extensions() -> vector<string> override { return {"gba"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto GameBoyAdvance::load(string location) -> bool {
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
  pak->setAttribute("title", document["game/title"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::load(node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=Flash,content=Save)"]) {
    Medium::load(node, ".flash");
    if(auto fp = pak->read("save.flash")) {
      fp->setAttribute("manufacturer", node["manufacturer"].string());
    }
  }

  bool mirror = false;
  if(auto node = document["game/board/memory(type=ROM,mirror=true)"]) {
    mirror = true;
  }
  pak->setAttribute("mirror", mirror);

  return true;
}

auto GameBoyAdvance::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::save(node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=Flash,content=Save)"]) {
    Medium::save(node, ".flash");
  }

  return true;
}

auto GameBoyAdvance::analyze(vector<u8>& rom) -> string {
  vector<string> identifiers = {
    "SRAM_V",
    "SRAM_F_V",
    "EEPROM_V",
    "FLASH_V",
    "FLASH512_V",
    "FLASH1M_V",
  };

  // TODO: Add remaining game codes
  vector<string> mirrorCodes = {
    "FSME",  // Super Mario Bros (US/EU)
  };

  string gameCode = "????";
  memory::copy(&gameCode, &rom[0xac], 4);
  string mirror = "false";

  for(auto& code : mirrorCodes) {
    if(!memory::compare(&gameCode, code.data(), code.size())) {
      mirror = "true";
    }
  }

  vector<string> list;
  for(auto& identifier : identifiers) {
    for(s32 n : range(rom.size() - 16)) {
      if(!memory::compare(&rom[n], identifier.data(), identifier.size())) {
        if(!list.find(identifier.data())) list.append(identifier.data());
      }
    }
  }

  string s;
  s += "game\n";
  s +={"  name:  ", Medium::name(location), "\n"};
  s +={"  title: ", Medium::name(location), "\n"};
  s += "  board\n";

  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  s +={"      mirror: ", mirror, "\n"};

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
