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
  pak->setAttribute("chip", document["game/board/chip/type"].string());
  pak->setAttribute("chip/key", document["game/board/chip/key"].natural());
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
  if(auto node = document["game/board/memory(type=ROM,content=Option)"]) {
    pak->append("option.rom", {view.data(), node["size"].natural()});
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
  if(data.size() < 256) {
    print("[mia] Loading rom failed. Minimum expected rom size is 256 (0x100) bytes. Rom size: ", data.size(), " (0x", hex(data.size()), ") bytes.\n");
    return {};
  }

  string digest = Hash::SHA256(data).digest();
  string manifest = Medium::manifestDatabase(digest);
  if(manifest) return manifest;

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
  string hash = Hash::SHA256({data.data() + 16, data.size() - 16}).digest();
  string manifest = Medium::manifestDatabase(hash);
  if(manifest) {
    manifest += "    memory\n";
    manifest += "      type: ROM\n";
    manifest += "      size: 0x10\n";
    manifest += "      content: iNES\n";
    manifest +={"      data: ", hexString({data.data(), 16}), "\n"};
    return manifest;
  }

  u32 mapper = ((data[7] >> 4) << 4) | (data[6] >> 4);
  u32 mirror = ((data[6] & 0x08) >> 2) | (data[6] & 0x01);
  u32 prgrom = data[4] * 0x4000;
  u32 chrrom = data[5] * 0x2000;
  u32 prgram = 0u;
  u32 chrram = chrrom == 0u ? 8192u : 0u;
  u32 eeprom = 0u;
  u32 submapper = 0u;

  string region = "NTSC-J, NTSC-U, PAL"; //iNES 1.0 requires database to detect region

  bool iNes2 = (data[7] & 0xc) == 0x8;
  if(iNes2) {
    mapper |= ((data[8] & 0xf) << 8);
    submapper = data[8] >> 4;
    u32 timing = data[12] & 3;

    // TODO: add DENDY (pirate famiclone) timing
    if(timing == 0) region = "NTSC-J, NTSC-U";
    if(timing == 1) region = "PAL";
    if(timing == 2) region = "NTSC-J, NTSC-U, PAL";
  }

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};

  switch(mapper) {

  default:
    debug(unimplemented, "[famicom] unknown iNES mapper number ", mapper);
    [[fallthrough]];
  case   0:
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
    s += "  board:  HVC-TLROM\n";
    s += "    chip type=MMC3B\n";
    prgram = 8192;
    break;

  case   5:
    s += "  board:  HVC-EWROM\n";
    s += "    chip type=MMC5\n";
    prgram = 32768;
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

  case  11:
    s += "  board:  COLORDREAMS-74*377\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  13:
    s += "  board:  HVC-CPROM\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    chrram = 16384;
    break;

  case  16:
    s += "  board:  BANDAI-LZ93D50\n";
    s += "    chip type=LZ93D50\n";
    eeprom = 256;
    break;

  case  18:
    s += "  board:  JALECO-JF-23A\n";
    s += "    chip type=SS88006\n";
    break;

  case  19:
    s += "  board:  NAMCO-163\n";
    s += "    chip type=163\n";
    prgram = 8192;
    break;

  case  21:
    switch(submapper) {
      case 0: case 1:
        s += "  board:  KONAMI-VRC-4\n";
        s += "    chip type=VRC4\n";
        s += "      pinout a0=1 a1=2\n";
        break;
      case 2:
        s += "  board:  KONAMI-VRC-4\n";
        s += "    chip type=VRC4\n";
        s += "      pinout a0=6 a1=7\n";
        break;
    }
    prgram = 8192;
    break;

  case  22:
    s += "  board:  KONAMI-VRC-2A\n";
    s += "    chip type=VRC2\n";
    s += "      pinout a0=1 a1=0\n";
    break;

  case  23:
    switch(submapper) {
      case 0: case 1:
        s += "  board:  KONAMI-VRC-4\n";
        s += "    chip type=VRC2\n";
        s += "      pinout a0=0 a1=1\n";
        break;
      case 2:
        s += "  board:  KONAMI-VRC-4\n";
        s += "    chip type=VRC2\n";
        s += "      pinout a0=2 a1=3\n";
        break;
      case 3:
        s += "  board:  KONAMI-VRC-2\n";
        s += "    chip type=VRC2\n";
        s += "      pinout a0=0 a1=1\n";
        break;
    }
    prgram = 8192;
    break;

  case  24:
    s += "  board:  KONAMI-VRC-6\n";
    s += "    chip type=VRC6\n";
    s += "      pinout a0=0 a1=1\n";
    break;

  case  25:
    switch(submapper) {
      case 0: case 1:
        s += "  board:  KONAMI-VRC-4\n";
        s += "    chip type=VRC4\n";
        s += "      pinout a0=1 a1=0\n";
        break;
      case 2:
        s += "  board:  KONAMI-VRC-4\n";
        s += "    chip type=VRC4\n";
        s += "      pinout a0=3 a1=2\n";
        break;
      case 3:
        s += "  board:  KONAMI-VRC-2\n";
        s += "    chip type=VRC2\n";
        s += "      pinout a0=1 a1=0\n";
        break;
    }

    prgram = 8192;
    break;

  case  26:
    s += "  board:  KONAMI-VRC-6\n";
    s += "    chip type=VRC6\n";
    s += "      pinout a0=1 a1=0\n";
    prgram = 8192;
    break;

  case  28:
    s += "  board:  ACTION53\n";
    break;

  case  30:
    s += "  board:  UNROM-512\n";
    s +={"    mirror mode=", mirror == 0 ? "horizontal" : (mirror == 1 ? "vertical" : (mirror == 2 ? "pcb" : "external")), "\n"};
    break;

  case  32:
    s += "  board:  IREM-G101\n";
    s += "    chip type=G101\n";
    prgram = 8192;
    break;

  case  33:
    s += "  board:  TAITO-TC0190\n";
    s += "    chip type=TC0190\n";
    break;

  case  34:
    if(submapper == 0 && chrrom != 0 || submapper == 1) {
      s += "  board:  AVE-NINA-001\n";
      prgram = 8192;
    } else {
      s += "  board:  HVC-BNROM\n";
      s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    }
    break;

  case  37:
    s += "  board:  PAL-ZZ\n";
    s += "    chip type=MMC3B\n";
    break;

  case  47:
    s += "  board:  NES-QJ\n";
    s += "    chip type=MMC3B\n";
    break;

  case  48:
    s += "  board:  TAITO-TC0690\n";
    s += "    chip type=TC0690\n";
    break;

  case  65:
    s += "  board:  IREM-H3001\n";
    s += "    chip type=H3001\n";
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
    prgram = 8192;
    break;

  case  69:
    s += "  board:  SUNSOFT-5B\n";
    s += "    chip type=5B\n";
    prgram = 8192;
    break;

  case  70:
    s += "  board:  BANDAI-74161\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  72:
    s += "  board:  JALECO-JF-17\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
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

  case  76:
    s += "  board:  NAMCO-3446\n";
    s += "    chip type=118\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  77:
    s += "  board:  IREM-LROG017\n";
    chrram = 8192;
    break;

  case  78:
    s += "  board:  JALECO-JF-16\n";
    break;

  case  79:
    s += "  board:  AVE-NINA-06\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  80:
    s += "  board:  TAITO-X1-005\n";
    s += "    chip type=X1-005\n";
    prgram = 128;
    break;

  case  82:
    s += "  board:  TAITO-X1-017\n";
    s += "    chip type=X1-017\n";
    prgram = 5120;
    break;

  case  85:
    s += "  board:  KONAMI-VRC-7\n";
    s += "    chip type=VRC7\n";
    s += "      pinout a0=4\n";
    prgram = 8192;
    break;

  case  86:
    s += "  board:  JALECO-JF-13\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  87:
    s += "  board:  JALECO-JF-05\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  88:
    s += "  board:  NAMCO-3433\n";
    s += "    chip type=118\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  89:
    s += "  board:  SUNSOFT-2\n";
    break;

  case  92:
    s += "  board:  JALECO-JF-19\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  93:
    s += "  board:  SUNSOFT-2\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  94:
    s += "  board:  HVC-UN1ROM\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case  95:
    s += "  board:  NAMCO-3425\n";
    s += "    chip type=118\n";
    break;

  case  96:
    s += "  board:  BANDAI-OEKA\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    chrram = 32768;
    break;

  case  97:
    s += "  board:  IREM-TAM-S1\n";
    s += "    chip type=TAM-S1\n";
    break;

  case 111:
    s += "  board:  GTROM\n";
    chrram = 16384;
    break;

  case 118:
    s += "  board:  HVC-TKSROM\n";
    s += "    chip type=MMC3B\n";
    prgram = 8192;
    break;

  case 119:
    s += "  board:  HVC-TQROM\n";
    s += "    chip type=MMC3B\n";
    chrram = 8192;
    break;

  case  140:
    s += "  board:  JALECO-JF-11\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case 152:
    s += "  board:  BANDAI-74161A\n";
    break;

  case 153:
    s += "  board:  BANDAI-LZ93D50\n";
    s += "    chip type=LZ93D50\n";
    prgram = 8192;
    break;

  case 154:
    s += "  board:  NAMCO-3453\n";
    s += "    chip type=118\n";
    break;

  case 155:
    s += "  board:  HVC-SXROM\n";
    s += "    chip type=MMC1A\n";
    prgram = 8192;
    break;

  case 157:
    s += "  board:  BANDAI-LZ93D50\n";
    s += "    chip type=LZ93D50\n";
    eeprom = 256;
    break;

  case 159:
    s += "  board:  BANDAI-LZ93D50\n";
    s += "    chip type=LZ93D50\n";
    eeprom = 128;
    break;

  case 180:
    s += "  board:  HVC-UNROMA\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case 184:
    s += "  board:  SUNSOFT-1\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case 185:
    s += "  board:  HVC-CNROM-SEC\n";
    s += "    chip type=SECURITY key=0x11\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case 188:
    s += "  board:  BANDAI-KARAOKE\n";
    break;

  case 206:
    s += "  board:  NAMCO-118\n";
    s += "    chip type=118\n";
    s +={"    mirror mode=", !mirror ? "horizontal" : "vertical", "\n"};
    break;

  case 207:
    s += "  board:  TAITO-X1-005A\n";
    s += "    chip type=X1-005\n";
    prgram = 128;
    break;

  case 210:
    s += "  board:  NAMCO-340\n";
    s += "    chip type=340\n";
    break;

  case 228:
    s += "  board:  MLT-ACTION52\n";
    break;
  }


  // iNES 2.0 overrides auto-detected ram amounts
  if(iNes2) {
    u32 chrshift = data[11] & 0xf;
    chrram = chrshift > 0 ? 64 << chrshift : 0;
  }

  s += "    memory\n";
  s += "      type: ROM\n";
  s += "      size: 0x10\n";
  s += "      content: iNES\n";
  s +={"      data: ", hexString({data.data(), 16}), "\n"};

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
