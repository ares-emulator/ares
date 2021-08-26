struct Famicom : Cartridge {
  auto name() -> string override { return "Famicom"; }
  auto extensions() -> vector<string> override { return {"fc", "nes", "unf", "unif"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& data) -> string;
  auto analyzeFDS(vector<u8>& data) -> string;
  auto analyzeINES(vector<u8>& data) -> string;
  auto analyzeUNIF(vector<u8>& data) -> string;
};

auto Famicom::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "ines.rom"});
    append(rom, {location, "program.rom"});
    append(rom, {location, "character.rom"});
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
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board", document["game/board"].string());
  pak->setAttribute("mirror", document["game/board/mirror/mode"].string());
  pak->setAttribute("pinout/a0", document["game/board/chip/pinout/a0"].natural());
  pak->setAttribute("pinout/a1", document["game/board/chip/pinout/a1"].natural());
  pak->append("manifest.bml", manifest);

  array_view<u8> view{rom};
  if(auto node = document["game/board/memory(type=ROM,content=iNES)"]) {
    pak->append("ines.rom", {view.data(), node["size"].natural()});
    view += node["size"].natural();
  }
  if(auto node = document["game/board/memory(type=ROM,content=Program)"]) {
    pak->append("program.rom", {view.data(), node["size"].natural()});
    view += node["size"].natural();
  }
  if(auto node = document["game/board/memory(type=ROM,content=Character)"]) {
    pak->append("character.rom", {view.data(), node["size"].natural()});
    view += node["size"].natural();
  }

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::load(node, ".eeprom");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Character)"]) {
    Medium::load(node, ".chr");
  }

  return true;
}

auto Famicom::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::save(node, ".eeprom");
  }

  return true;
}

auto Famicom::analyze(vector<u8>& data) -> string {
  if(data.size() < 256) return {};

  string digest = Hash::SHA256(data).digest();

  if(digest == "99c18490ed9002d9c6d999b9d8d15be5c051bdfa7cc7e73318053c9a994b0178"  //Nintendo Famicom Disk System (Japan)
  || digest == "a0a9d57cbace21bf9c85c2b85e86656317f0768d7772acc90c7411ab1dbff2bf"  //Sharp Twin Famicom (Japan)
  ) {
    return analyzeFDS(data);
  }

  if(data[0] == 'N' && data[1] == 'E' && data[2] == 'S' && data[3] == 0x1a) {
    return analyzeINES(data);
  }

  if(data[0] == 'U' && data[1] == 'N' && data[2] == 'I' && data[3] == 'F') {
    return analyzeUNIF(data);
  }

  //unsupported format
  return {};
}

//Famicom Disk System (BIOS)
auto Famicom::analyzeFDS(vector<u8>& data) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s += "  region: NTSC-J\n";
  s += "  board:  HVC-FMR\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s += "      size: 0x2000\n";
  s += "      content: Program\n";
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x8000\n";
  s += "      content: Save\n";
  s += "      volatile\n";
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x2000\n";
  s += "      content: Character\n";
  s += "      volatile\n";
  return s;
}

//iNES
auto Famicom::analyzeINES(vector<u8>& data) -> string {
  u32 mapper = ((data[7] >> 4) << 4) | (data[6] >> 4);
  u32 mirror = ((data[6] & 0x08) >> 2) | (data[6] & 0x01);
  u32 prgrom = data[4] * 0x4000;
  u32 chrrom = data[5] * 0x2000;
  u32 prgram = 0u;
  u32 chrram = chrrom == 0u ? 8192u : 0u;
  u32 eeprom = 0u;
  string hash = Hash::SHA256({data.data() + 16, data.size() - 16}).digest();

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s += "  region: NTSC-J, NTSC-U, PAL\n";  //database required to detect region

  //Family BASIC (Japan)
  if(hash == "c8c0b6c21bdda7503bab7592aea0f945a0259c18504bb241aafb1eabe65846f3") {
    prgram = 8_KiB;
  }

  //Gauntlet (USA)
  if(hash == "fd2a8520314fb183e15fd62f48df97f92eb9c81140da4e6ab9ff0386e4797071") {
    mapper = 206;
  }

  //Gauntlet (USA) [Unlicensed]
  if(hash == "67b8a39744807dd740bdebcfe3d33bdac11a4d47b4807c0ffd35e322f8d670c2") {
    mapper = 206;
  }

  //Jajamaru Gekimaden: Maboroshi no Kinmajou (Japan)
  if(hash == "ea770788f68e4bb089e4205807931d64b83175a0106e7563d0a6d3ebac369991") {
    prgram = 8_KiB;
  }

  //Mezase Top Pro: Green ni Kakeru Yume (Japan)
  if(hash == "32c6893e0f8a14714dd2082803cfde62f8981010d23fc9cf00a5a18066d063cb") {
    prgram = 8_KiB;
  }

  switch(mapper) {

  default:
    s += "  board:  HVC-NROM-256\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case   1:
    s += "  board:  HVC-SXROM\n";
    s += "    chip type=MMC1B2\n";
    prgram = 8192;
    break;

  case   2:
    s += "  board:  HVC-UOROM\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case   3:
    s += "  board:  HVC-CNROM\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case   4:
    //MMC3
    s += "  board:  HVC-TLROM\n";
    s += "    chip type=MMC3B\n";
    prgram = 8192;
    //MMC6
  //s += "  board:  HVC-HKROM\n";
  //s += "    chip type=MMC6\n";
  //prgram = 1024;
    break;

  case   5:
    s += "  board:  HVC-ELROM\n";
    s += "    chip type=MMC5\n";
    prgram = 65536;
    break;

  case   7:
    s += "  board:  HVC-AOROM\n";
    break;

  case   9:
    s += "  board:  HVC-PNROM\n";
    s += "    chip type=MMC2\n";
    prgram = 8192;
    break;

  case  10:
    s += "  board:  HVC-FKROM\n";
    s += "    chip type=MMC4\n";
    prgram = 8192;
    break;

  case  16:
    s += "  board:  BANDAI-FCG\n";
    s += "    chip type=LZ93D50\n";
    eeprom = 128;
    break;

  case  18:
    s += "  board:  JALECO-JF\n";
    s += "    chip type=SS88006\n";
    break;

  case  21:
  case  23:
  case  25:
    //VRC4
    s += "  board:  KONAMI-VRC-4\n";
    s += "    chip type=VRC4\n";
    s += "      pinout a0=1 a1=0\n";
    prgram = 8192;
    break;

  case  22:
    //VRC2
    s += "  board:  KONAMI-VRC-2\n";
    s += "    chip type=VRC2\n";
    s += "      pinout a0=0 a1=1\n";
    break;

  case  24:
    s += "  board:  KONAMI-VRC-6\n";
    s += "    chip type=VRC6\n";
    s += "      pinout a0=0 a1=1\n";
    break;

  case  26:
    s += "  board:  KONAMI-VRC-6\n";
    s += "    chip type=VRC6\n";
    s += "      pinout a0=1 a1=0\n";
    prgram = 8192;
    break;

  case  34:
    s += "  board:  HVC-BNROM\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  66:
    s += "  board:  HVC-GNROM\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  67:
    s += "  board:  SUNSOFT-3\n";
    break;

  case  68:
    s += "  board:  SUNSOFT-4\n";
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x4000\n";
    s += "      content: Option\n";
    prgram = 8192;
    break;

  case  69:
    s += "  board:  SUNSOFT-5B\n";
    s += "    chip type=5B\n";
    prgram = 8192;
    break;

  case  73:
    s += "  board:  KONAMI-VRC-3\n";
    s += "    chip type=VRC3\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    prgram = 8192;
    break;
    
  case  75:
    s += "  board:  KONAMI-VRC-1\n";
    s += "    chip type=VRC1\n";
    break;

  case  85:
    s += "  board:  KONAMI-VRC-7\n";
    s += "    chip type=VRC7\n";
    prgram = 8192;
    break;

  case  89:
    s += "  board:  SUNSOFT-2\n";
    break;

  case  93:
    s += "  board:  SUNSOFT-2\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  97:
    s += "  board:  IREM-TAM-S1\n";
    break;

  case  140:
    s += "  board:  JALECO-JF11-JF14\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case 159:
    s += "  board:  BANDAI-FCG\n";
    s += "    chip type=LZ93D50\n";
    eeprom = 128;
    break;

  case 184:
    s += "  board:  SUNSOFT-1\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case 206:
    s += "  board: HVC-DRROM\n";
    chrram = 2048;
    break;

  }

  s += "    memory\n";
  s += "      type: ROM\n";
  s += "      size: 0x10\n";
  s += "      content: iNES\n";

  if(prgrom) {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(prgrom), "\n"};
    s += "      content: Program\n";
  }

  if(prgram) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(prgram), "\n"};
    s += "      content: Save\n";
  }

  if(chrrom) {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(chrrom), "\n"};
    s += "      content: Character\n";
  }

  if(chrram) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(chrram), "\n"};
    s += "      content: Character\n";
    s += "      volatile\n";
  }

  if(eeprom) {
    s += "    memory\n";
    s += "      type: EEPROM\n";
    s +={"      size: 0x", hex(eeprom), "\n"};
    s += "      content: Save\n";
  }

  return s;
}

auto Famicom::analyzeUNIF(vector<u8>& data) -> string {
  string board;
  string region = "NTSC";  //fallback
  bool battery = false;
  string mirroring;
  vector<u8> programROMs[8];
  vector<u8> characterROMs[8];

  u32 offset = 32;
  while(offset + 8 < data.size()) {
    string type;
    type.resize(4);
    memory::copy(type.get(), &data[offset + 0], 4);

    u32 size = 0;
    size |= data[offset + 4] <<  0;
    size |= data[offset + 5] <<  8;
    size |= data[offset + 6] << 16;
    size |= data[offset + 7] << 24;

    //will attempting to read this block go out of bounds?
    if(offset + size + 8 > data.size()) break;

    if(type == "MAPR") {
      board.resize(size);
      memory::copy(board.get(), &data[offset + 8], size);
      if(!board[size - 1]) board.resize(size - 1);  //remove unnecessary null-terminator
    }

    if(type == "TVCI" && size > 0) {
      u8 byte = data[offset + 8];
      if(byte == 0x00) region = "NTSC";
      if(byte == 0x01) region = "PAL";
      if(byte == 0x02) region = "NTSC, PAL";
    }

    if(type == "BATR" && size > 0) {
      u8 byte = data[offset + 8];
      if(byte == 0x00) battery = false;
      if(byte == 0x01) battery = true;
    }

    if(type == "MIRR" && size > 0) {
      u8 byte = data[offset + 8];
      if(byte == 0x00) mirroring = "A11";  //horizontal
      if(byte == 0x01) mirroring = "A10";  //vertical
      if(byte == 0x02) mirroring = "GND";  //screen A
      if(byte == 0x03) mirroring = "VCC";  //screen B
      if(byte == 0x04) mirroring = "EXT";  //four-screen
      if(byte == 0x05) mirroring = "PCB";  //mapper-controlled
    }

    if(type.beginsWith("PRG")) {
      u8 id = data[offset + 3] - '0';
      if(id >= 8) continue;  //invalid ID
      programROMs[id].resize(size);
      memory::copy(programROMs[id].data(), &data[offset + 8], size);
    }

    if(type.beginsWith("CHR")) {
      u8 id = data[offset + 3] - '0';
      if(id >= 8) continue;  //invalid ID
      characterROMs[id].resize(size);
      memory::copy(characterROMs[id].data(), &data[offset + 8], size);
    }

    offset += 8 + size;
  }

  vector<u8> programROM;
  vector<u8> characterROM;
  for(u32 id : range(8)) programROM.append(programROMs[id]);
  for(u32 id : range(8)) characterROM.append(characterROMs[id]);

  u32 programRAM = 0;
  u32 characterRAM = 0;

  if(board == "KONAMI-QTAI") {
    board = "KONAMI-VRC-5";
    programRAM = 8_KiB + 8_KiB;
    characterRAM = 8_KiB;
  }

  //ensure required chucks were found
  if(!board) return {};
  if(!programROM) return {};

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  s +={"  board:  ", board, "\n"};
  if(mirroring) {
    s +={"    mirror mode=", mirroring, "\n"};
  }
  if(programROM) {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(programROM.size()), "\n"};
    s += "      content: Program\n";
  }
  if(programRAM) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(programRAM), "\n"};
    s += "      content: Save\n";
    if(!battery) {
      s += "      volatile\n";
    }
  }
  if(characterROM) {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(characterROM.size()), "\n"};
    s += "      content: Character\n";
  }
  if(characterRAM) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(characterRAM), "\n"};
    s += "      content: Character\n";
    s += "      volatile\n";
  }

  data.reset();
  data.append(programROM);
  data.append(characterROM);

  return s;
}
