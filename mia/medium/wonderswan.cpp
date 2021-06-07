struct WonderSwan : Cartridge {
  auto name() -> string override { return "WonderSwan"; }
  auto extensions() -> vector<string> override { return {"ws"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
  auto analyzeHacks(vector<u8>& rom, string hash) -> void;
};

auto WonderSwan::load(string location) -> bool {
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
  pak->setAttribute("orientation", document["game/orientation"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::load(node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Medium::load(node, ".rtc");
  }

  return true;
}

auto WonderSwan::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::save(node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Medium::save(node, ".rtc");
  }

  return true;
}

auto WonderSwan::analyze(vector<u8>& rom) -> string {
  if(rom.size() < 0x10000) return {};

  auto hash = Hash::SHA256(rom).digest();
  analyzeHacks(rom, hash);

  auto metadata = &rom[rom.size() - 16];

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
  s +={"  sha256:      ", hash, "\n"};
  s +={"  name:        ", Medium::name(location), "\n"};
  s +={"  title:       ", Medium::name(location), "\n"};
  s +={"  orientation: ", !orientation ? "horizontal" : "vertical", "\n"};
  s += "  board\n";

  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
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

auto WonderSwan::analyzeHacks(vector<u8>& rom, string hash) -> void {
  //Meitantei Conan: Nishi no Meitantei Saidai no Kiki! (Japan)
  if(hash == "183ab5ec58beb95d460211fa09fa968c6f01f80f1ae2b78b5b11542f22ddd95a") {
    //changes the active ROM bank within said bank.
    //bank target should be 0x0f, but is 0x0e.
    //bank 0x0e (0fxxxx) contains 0xff bytes.
    //bank 0x0f (1fxxxx) contains vectors.
    //0fffe0  mov al,0xe0
    //0fffe2  out 0xc2,al
    //0fffe4  mov al,0x0e
    //0fffe6  out 0xc0,al        ;jumps to 0fffe8 (efffe8)
    //1fffe8  jmp 0x2000:0x0000  ;but should jump to 1fffe8 (ffffe8)
    for(auto address : range(32)) {
      rom[0x0fffe0 + address] = rom[0x1fffe0 + address];
    }
  }
}
