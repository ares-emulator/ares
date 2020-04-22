struct Famicom : Cartridge {
  auto name() -> string override { return "Famicom"; }
  auto extensions() -> vector<string> override { return {"nes", "unif"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
  auto heuristicsFDS(vector<uint8_t>& data, string location) -> string;
  auto heuristicsINES(vector<uint8_t>& data, string location) -> string;
  auto heuristicsUNIF(vector<uint8_t>& data, string location) -> string;
};

auto Famicom::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "ines.rom"});
  append(data, {location, "unif.rom"});
  append(data, {location, "program.rom"});
  append(data, {location, "character.rom"});
  return data;
}

auto Famicom::heuristics(vector<uint8_t>& data, string location) -> string {
  if(data.size() < 256) return {};

  string digest = Hash::SHA256(data).digest();

  if(digest == "99c18490ed9002d9c6d999b9d8d15be5c051bdfa7cc7e73318053c9a994b0178"  //Nintendo Famicom Disk System (Japan)
  || digest == "a0a9d57cbace21bf9c85c2b85e86656317f0768d7772acc90c7411ab1dbff2bf"  //Sharp Twin Famicom (Japan)
  ) {
    return heuristicsFDS(data, location);
  }

  if(data[0] == 'N' && data[1] == 'E' && data[2] == 'S' && data[3] == 0x1a) {
    return heuristicsINES(data, location);
  }

  if(data[0] == 'U' && data[1] == 'N' && data[2] == 'I' && data[3] == 'F') {
    return heuristicsUNIF(data, location);
  }

  //unsupported format
  return {};
}

//Famicom Disk System (BIOS)
auto Famicom::heuristicsFDS(vector<uint8_t>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s += "  board: HVC-FMR\n";
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
auto Famicom::heuristicsINES(vector<uint8_t>& data, string location) -> string {
  uint mapper = ((data[7] >> 4) << 4) | (data[6] >> 4);
  uint mirror = ((data[6] & 0x08) >> 2) | (data[6] & 0x01);
  uint prgrom = data[4] * 0x4000;
  uint chrrom = data[5] * 0x2000;
  uint prgram = 0u;
  uint chrram = chrrom == 0u ? 8192u : 0u;
  uint eeprom = 0u;

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};

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

  case 159:
    s += "  board:  BANDAI-FCG\n";
    s += "    chip type=LZ93D50\n";
    eeprom = 128;
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

auto Famicom::heuristicsUNIF(vector<uint8_t>& data, string location) -> string {
  string board;
  string region = "NTSC";  //fallback
  bool battery = false;
  string mirroring;
  vector<uint8_t> programROMs[8];
  vector<uint8_t> characterROMs[8];

  uint offset = 32;
  while(offset + 8 < data.size()) {
    string type;
    type.resize(4);
    memory::copy(type.get(), &data[offset + 0], 4);

    uint32_t size = 0;
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
      uint8_t byte = data[offset + 8];
      if(byte == 0x00) region = "NTSC";
      if(byte == 0x01) region = "PAL";
      if(byte == 0x02) region = "NTSC, PAL";
    }

    if(type == "BATR" && size > 0) {
      uint8_t byte = data[offset + 8];
      if(byte == 0x00) battery = false;
      if(byte == 0x01) battery = true;
    }

    if(type == "MIRR" && size > 0) {
      uint8_t byte = data[offset + 8];
      if(byte == 0x00) mirroring = "A11";  //horizontal
      if(byte == 0x01) mirroring = "A10";  //vertical
      if(byte == 0x02) mirroring = "GND";  //screen A
      if(byte == 0x03) mirroring = "VCC";  //screen B
      if(byte == 0x04) mirroring = "EXT";  //four-screen
      if(byte == 0x05) mirroring = "PCB";  //mapper-controlled
    }

    if(type.beginsWith("PRG")) {
      uint8_t id = data[offset + 3] - '0';
      if(id >= 8) continue;  //invalid ID
      programROMs[id].resize(size);
      memory::copy(programROMs[id].data(), &data[offset + 8], size);
    }

    if(type.beginsWith("CHR")) {
      uint8_t id = data[offset + 3] - '0';
      if(id >= 8) continue;  //invalid ID
      characterROMs[id].resize(size);
      memory::copy(characterROMs[id].data(), &data[offset + 8], size);
    }

    offset += 8 + size;
  }

  vector<uint8_t> programROM;
  vector<uint8_t> characterROM;
  for(uint id : range(8)) programROM.append(programROMs[id]);
  for(uint id : range(8)) characterROM.append(characterROMs[id]);

  uint programRAM = 0;
  uint characterRAM = 0;

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
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s +={"  board: ", board, "\n"};
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
