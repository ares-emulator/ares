struct GameBoyAdvance : Cartridge {
  auto name() -> string override { return "Game Boy Advance"; }
  auto extensions() -> vector<string> override { return {"gba"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto GameBoyAdvance::load(string location) -> LoadResult {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return romNotFound;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

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

  return successful;
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

  vector<string> mirrorCodes = {
    "FBME",  //Classic NES Series - Bomberman (USA, Europe)
    "FADE",  //Classic NES Series - Castlevania (USA)
    "FDKE",  //Classic NES Series - Donkey Kong (USA, Europe)
    "FEBE",  //Classic NES Series - Excitebike (USA, Europe)
    "FICE",  //Classic NES Series - Ice Climber (USA, Europe)
    "FSME",  //Classic NES Series - Super Mario Bros. (USA, Europe)
    "FZLE",  //Classic NES Series - The Legend of Zelda (USA, Europe)
    "FDME",  //Classic NES Series - Dr. Mario (USA, Europe)
    "FMRE",  //Classic NES Series - Metroid (USA, Europe)
    "FP7E",  //Classic NES Series - Pac-Man (USA, Europe)
    "FXVE",  //Classic NES Series - Xevious (USA, Europe)
    "FLBE",  //Classic NES Series - Zelda II - The Adventure of Link (USA, Europe)
    "FSRJ",  //Famicom Mini - Dai-2-ji Super Robot Taisen (Japan) (Promo)
    "FGZJ",  //Famicom Mini - Kidou Senshi Z Gundam - Hot Scramble (Japan) (Promo)
    "FSMJ",  //Famicom Mini 01 - Super Mario Bros. (Japan) (En) (Rev 1)
    "FDKJ",  //Famicom Mini 02 - Donkey Kong (Japan) (En)
    "FICJ",  //Famicom Mini 03 - Ice Climber (Japan) (En)
    "FEBJ",  //Famicom Mini 04 - Excitebike (Japan) (En)
    "FZLJ",  //Famicom Mini 05 - Zelda no Densetsu 1 - The Hyrule Fantasy (Japan)
    "FPMJ",  //Famicom Mini 06 - Pac-Man (Japan) (En)
    "FXVJ",  //Famicom Mini 07 - Xevious (Japan) (En)
    "FMPJ",  //Famicom Mini 08 - Mappy (Japan) (En)
    "FBMJ",  //Famicom Mini 09 - Bomberman (Japan) (En)
    "FSOJ",  //Famicom Mini 10 - Star Soldier (Japan) (En)
    "FMBJ",  //Famicom Mini 11 - Mario Bros. (Japan)
    "FCLJ",  //Famicom Mini 12 - Clu Clu Land (Japan)
    "FBFJ",  //Famicom Mini 13 - Balloon Fight (Japan)
    "FWCJ",  //Famicom Mini 14 - Wrecking Crew (Japan)
    "FDMJ",  //Famicom Mini 15 - Dr. Mario (Japan)
    "FDDJ",  //Famicom Mini 16 - Dig Dug (Japan)
    "FTBJ",  //Famicom Mini 17 - Takahashi Meijin no Bouken-jima (Japan)
    "FMKJ",  //Famicom Mini 18 - Makaimura (Japan)
    "FTWJ",  //Famicom Mini 19 - Twin Bee (Japan)
    "FGGJ",  //Famicom Mini 20 - Ganbare Goemon! - Karakuri Douchuu (Japan)
    "FM2J",  //Famicom Mini 21 - Super Mario Bros. 2 (Japan)
    "FADJ",  //Famicom Mini 29 - Akumajou Dracula (Japan)
    "FSDJ",  //Famicom Mini 30 - SD Gundam World - Gachapon Senshi Scramble Wars (Japan)
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
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  sha256: ", sha256, "\n"};
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
