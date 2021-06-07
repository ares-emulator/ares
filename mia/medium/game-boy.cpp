struct GameBoy : Cartridge {
  auto name() -> string override { return "Game Boy"; }
  auto extensions() -> vector<string> override { return {"gb"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto GameBoy::load(string location) -> bool {
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
  pak->setAttribute("board", document["game/board"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::load(node, ".eeprom");
    if(auto fp = pak->read("save.eeprom")) {
      fp->setAttribute("width", node["width"].natural());
    }
  }
  if(auto node = document["game/board/memory(type=Flash,content=Download)"]) {
    Medium::load(node, ".flash");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Medium::load(node, ".rtc");
  }

  return true;
}

auto GameBoy::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::save(node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=Flash,content=Download)"]) {
    Medium::save(node, ".flash");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Medium::save(node, ".rtc");
  }

  return true;
}

auto GameBoy::analyze(vector<u8>& rom) -> string {
  if(rom.size() < 0x4000) return {};

  auto hash = Hash::SHA256(rom).digest();

  u32 headerAddress = rom.size() < 0x8000 ? rom.size() : rom.size() - 0x8000;
  auto read = [&](u32 offset) { return rom[headerAddress + offset]; };

  if(read(0x0104) == 0xce && read(0x0105) == 0xed && read(0x0106) == 0x66 && read(0x0107) == 0x66
  && read(0x0108) == 0xcc && read(0x0109) == 0x0d && read(0x0147) >= 0x0b && read(0x0147) <= 0x0d
  ) {
    //MMM01 stores header at bottom of rom[]
  } else {
    //all other mappers store header at top of rom[]
    headerAddress = 0;
  }

  bool black = (read(0x0143) & 0xc0) == 0x80;  //cartridge works in DMG+CGB mode
  bool clear = (read(0x0143) & 0xc0) == 0xc0;  //cartridge works in CGB mode only

  bool ram = false;
  bool battery = false;
  bool eeprom = false;
  bool flash = false;
  bool rtc = false;
  bool accelerometer = false;
  bool rumble = false;

  u32 romSize = 0;
  u32 ramSize = 0;
  u32 eepromSize = 0;
  u32 eepromWidth = 0;
  u32 flashSize = 0;
  u32 rtcSize = 0;

  string mapper;

  switch(read(0x0147)) {
  case 0x00:
    //no mapper
    break;

  case 0x01:
    mapper = "MBC1";
    break;

  case 0x02:
    mapper = "MBC1";
    ram = true;
    break;

  case 0x03:
    mapper = "MBC1";
    battery = true;
    ram = true;
    break;

  case 0x05:
    mapper = "MBC2";
    ram = true;
    break;

  case 0x06:
    mapper = "MBC2";
    battery = true;
    ram = true;
    break;

  case 0x08:
    mapper = "MBC0";
    ram = true;
    break;

  case 0x09:
    mapper = "MBC0";
    battery = true;
    ram = true;
    break;

  case 0x0b:
    mapper = "MMM01";
    break;

  case 0x0c:
    mapper = "MMM01";
    ram = true;
    break;

  case 0x0d:
    mapper = "MMM01";
    battery = true;
    ram = true;
    break;

  case 0x0f:
    mapper = "MBC3";
    battery = true;
    rtc = true;
    break;

  case 0x10:
    mapper = "MBC3";
    battery = true;
    ram = true;
    rtc = true;
    break;

  case 0x11:
    mapper = "MBC3";
    break;

  case 0x12:
    mapper = "MBC3";
    ram = true;
    break;

  case 0x13:
    mapper = "MBC3";
    battery = true;
    ram = true;
    break;

  case 0x19:
    mapper = "MBC5";
    break;

  case 0x1a:
    mapper = "MBC5";
    ram = true;
    break;

  case 0x1b:
    mapper = "MBC5";
    battery = true;
    ram = true;
    break;

  case 0x1c:
    mapper = "MBC5";
    rumble = true;
    break;

  case 0x1d:
    mapper = "MBC5";
    ram = true;
    rumble = true;
    break;

  case 0x1e:
    mapper = "MBC5";
    battery = true;
    ram = true;
    rumble = true;
    break;

  case 0x20:
    mapper = "MBC6";
    flash = true;
    battery = true;
    ram = true;
    break;

  case 0x22:
    mapper = "MBC7";
    battery = true;
    eeprom = true;
    accelerometer = true;
    rumble = true;
    break;

  case 0xfc:
    mapper = "CAMERA";
    break;

  case 0xfd:
    mapper = "TAMA";
    battery = true;
    ram = true;
    rtc = true;
    break;

  case 0xfe:
    mapper = "HuC3";
    break;

  case 0xff:
    mapper = "HuC1";
    battery = true;
    ram = true;
    break;
  }

  //Game Boy: label = $0134-0143
  //Game Boy Color (early games): label = $0134-0142; model = $0143
  //Game Boy Color (later games): label = $0134-013e; serial = $013f-0142; model = $0143
  string label;
  for(u32 n : range(black || clear ? 15 : 16)) {
    char byte = read(0x0134 + n);
    if(byte < 0x20 || byte > 0x7e) byte = ' ';
    label.append(byte);
  }

  string serial = label.slice(-4);
  if(!black && !clear) serial = "";
  for(auto& byte : serial) {
    if(byte >= 'A' && byte <= 'Z') continue;
    //invalid serial
    serial = "";
    break;
  }
  label.trimRight(serial, 1L);  //remove the serial from the label, if it exists
  label.strip();  //remove any excess whitespace from the label

  switch(read(0x0148)) { default:
  case 0x00: romSize =   2 * 16 * 1024; break;
  case 0x01: romSize =   4 * 16 * 1024; break;
  case 0x02: romSize =   8 * 16 * 1024; break;
  case 0x03: romSize =  16 * 16 * 1024; break;
  case 0x04: romSize =  32 * 16 * 1024; break;
  case 0x05: romSize =  64 * 16 * 1024; break;
  case 0x06: romSize = 128 * 16 * 1024; break;
  case 0x07: romSize = 256 * 16 * 1024; break;
  case 0x52: romSize =  72 * 16 * 1024; break;
  case 0x53: romSize =  80 * 16 * 1024; break;
  case 0x54: romSize =  96 * 16 * 1024; break;
  }

  switch(read(0x0149)) { default:
  case 0x00: ramSize =   0_KiB; break;
  case 0x01: ramSize =   2_KiB; break;
  case 0x02: ramSize =   8_KiB; break;
  case 0x03: ramSize =  32_KiB; break;
  case 0x04: ramSize = 128_KiB; break;
  case 0x05: ramSize =  64_KiB; break;
  }

  //Bomber Man Collection (Japan)
  if(hash == "85f6b7b2eb182f53b00ac3e9e4a86ef8f7496550a0b2f64573ab61f670e30245") {
    mapper = "MBC1#M";
  }

  //Genjin Collection (Japan)
  if(hash == "40c3b05b76355d9a4e244cb22c074648f03933571b023f77f667e8b2e4b48513") {
    mapper = "MBC1#M";
  }

  //Momotarou Collection (Japan)
  if(hash == "db3a45363490b8cb56274f260358c7b8dc0ffc0a41517123d6c5d78c2c54dc38") {
    mapper = "MBC1#M";
  }

  //Mortal Kombat I & II (Japan)
  if(hash == "c6c43dce0514016a4a635db3ec250bfa5790422042599457cd30841ec92f5979") {
    mapper = "MBC1#M";
  }

  //Mortal Kombat I & II (USA, Europe)
  if(hash == "cacace0974a588c68766bbf21a631be9fa234335e0607ebfa9de77b3dd0cbf18") {
    mapper = "MBC1#M";
  }

  if(mapper == "MBC2" && ram) ramSize = 256;
  if(mapper == "MBC3" && (romSize > 2_MiB || ramSize > 32_KiB)) mapper = "MBC30";
  if(mapper == "MBC6" && ram) ramSize =  32_KiB;
  if(mapper == "TAMA" && ram) ramSize =  32;
  if(mapper == "MBC6" && flash) flashSize = 1_MiB;

  //Game Boy header does not specify EEPROM size: detect via game label instead
  //Command Master:        EEPROM = 512 bytes
  //Kirby Tilt 'n' Tumble: EEPROM = 256 bytes
  //Korokoro Kirby:        EEPROM = 256 bytes
  if(mapper == "MBC7" && eeprom) {
    eepromSize = 256;  //fallback guess; supported values are 128, 256, 512, 1024, 2048
    eepromWidth = 16;  //93LCx6 supports 8-bit mode, but no licensed games use it
    if(label == "CMASTER"     && serial == "KCEJ") eepromSize = 512;
    if(label == "KIRBY TNT"   && serial == "KTNE") eepromSize = 256;
    if(label == "KORO2 KIRBY" && serial == "KKKJ") eepromSize = 256;
  }

  if(mapper == "MBC3" && rtc) rtcSize = 13;
  if(mapper == "TAMA" && rtc) rtcSize = 15;

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  label:  ", label, "\n"};
  if(serial)
  s +={"  serial: ", serial, "\n"};
  if(mapper)
  s +={"  board:  ", mapper, "\n"};
  else
  s += "  board\n";

  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";

  if(ram && ramSize) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(ramSize), "\n"};
    s += "      content: Save\n";
    if(!battery)
    s += "      volatile\n";
  }

  if(eeprom && eepromSize) {
    s += "    memory\n";
    s += "      type: EEPROM\n";
    s +={"      size: 0x", hex(eepromSize), "\n"};
    s +={"      width: ", eepromWidth, "\n"};
    s += "      content: Save\n";
  }

  if(flash && flashSize) {
    s += "    memory\n";
    s += "      type: Flash\n";
    s +={"      size: 0x", hex(flashSize), "\n"};
    s += "      content: Download\n";
  }

  if(rtc && rtcSize) {
    s += "    memory\n";
    s += "      type: RTC\n";
    s +={"      size: 0x", hex(rtcSize), "\n"};
    s += "      content: Time\n";
  }

  if(accelerometer)
  s += "    accelerometer\n";

  if(rumble)
  s += "    rumble\n";

  return s;
}
