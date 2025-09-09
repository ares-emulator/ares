struct Nintendo64DD : FloppyDisk {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto extensions() -> std::vector<string> override { return {"n64dd", "ndd", "d64"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom, std::vector<u8> errorTable) -> string;
  auto transform(std::span<const u8> input, std::vector<u8> errorTable) -> std::vector<u8>;
  auto sizeCheck(std::span<const u8> input) -> bool;
  auto repeatCheck(std::span<const u8> input, u32 repeat, u32 size) -> bool;
  auto createErrorTable(std::span<const u8> input) -> std::vector<u8>;
};

auto Nintendo64DD::load(string location) -> LoadResult {
  std::vector<u8> input;
  if(directory::exists(location)) {
    append(input, {location, "program.disk"});
  } else if(file::exists(location)) {
    input = FloppyDisk::read(location);
  }
  if(input.empty()) return romNotFound;

  std::span<const u8> view{input};
  auto errorTable = createErrorTable(view);
  if(errorTable.empty()) return invalidROM;
  auto sizeValid = sizeCheck(view);
  if(!sizeValid) return invalidROM;
  this->location = location;
  this->manifest = analyze(input, errorTable);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;
  pak = shared_pointer{new vfs::directory};
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.disk.error", errorTable);

  auto output = transform(view, errorTable);
  if(!output.empty()) {
    pak->append("program.disk", output);
  }

  if(!pak) return otherError;

  Pak::load("program.disk", ".disk");

  return successful;
}

auto Nintendo64DD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  Pak::save("program.disk", ".disk");

  return true;
}

auto Nintendo64DD::analyze(std::vector<u8>& rom, std::vector<u8> errorTable) -> string {
  //basic disk format check (further d64 check will be done later)
  b1 ndd = (rom.size() == 0x3DEC800);
  b1 mame = (rom.size() == 0x435B0C0);
  b1 d64 = (rom.size() >= 0x4F08) && (rom.size() < 0x3D79140);

  b1 dev = errorTable[12] == 0;
  string region = "NTSC-J";
  u32 systemBlocks[4] = {0, 1, 8, 9};
  for(u32 n : range(4)) {
    if(dev) {
      if(errorTable[systemBlocks[n]+2]) continue;
      region = "NTSC-DEV";
    } else {
      if(errorTable[systemBlocks[n]]) continue;
      u32 systemOffset = systemBlocks[n] * 0x4D08;
      if(rom[systemOffset] == 0xE8) region = "NTSC-J";
      else region = "NTSC-U";
    }
    break;
  }

  string id;
  u32 diskIdBlocks[2] = {14, 15};
  for(u32 n : range(2)) {
    if(errorTable[diskIdBlocks[n]]) continue;
    u32 diskIdOffset = diskIdBlocks[n] * 0x4D08;
    if(d64) diskIdOffset = 0x100;

    id.append((char)rom[diskIdOffset + 0x00]);
    id.append((char)rom[diskIdOffset + 0x01]);
    id.append((char)rom[diskIdOffset + 0x02]);
    id.append((char)rom[diskIdOffset + 0x03]);
    break;
  }

  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  s +={"  id:     ", id, "\n"};
  return s;
}

auto Nintendo64DD::sizeCheck(std::span<const u8> input) -> bool {
  //check disk image size
  //ndd
  if(input.size() == 0x3DEC800) return true;
  //mame
  if(input.size() == 0x435B0C0) return true;
  //d64
  if((input.size() >= 0x4F08) && (input.size() < 0x3D79140)) {
    u32 size = 0x200;
    u32 type = input[0x05];
    u32 rom_start_lba = 24;
    u32 rom_end_lba = (input[0xE0] << 8) + input[0xE1] + 24;
    u32 ram_start_lba = (input[0xE2] << 8) + input[0xE3];
    u32 ram_end_lba = (input[0xE4] << 8) + input[0xE5];
    //either both are 0xFFFF or aren't
    if((ram_start_lba == 0xFFFF) && (ram_end_lba != 0xFFFF)) return false;
    if((ram_start_lba != 0xFFFF) && (ram_end_lba == 0xFFFF)) return false;
    //add base lba
    if((ram_start_lba != 0xFFFF) && (ram_end_lba != 0xFFFF)) {
      ram_start_lba += 24;
      ram_end_lba += 24;
    }

    u32 blockSizeTable[9] = {0x4D08, 0x47B8, 0x4510, 0x3FC0, 0x3A70, 0x3520, 0x2FD0, 0x2A80, 0x2530};
    u16 vzoneLbaTable[7][16] = {
      {0x0124, 0x0248, 0x035A, 0x047E, 0x05A2, 0x06B4, 0x07C6, 0x08D8, 0x09EA, 0x0AB6, 0x0B82, 0x0C94, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
      {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x06A2, 0x07C6, 0x08D8, 0x09EA, 0x0AFC, 0x0BC8, 0x0C94, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
      {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08C6, 0x09EA, 0x0AFC, 0x0C0E, 0x0CDA, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
      {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08B4, 0x09C6, 0x0AEA, 0x0C0E, 0x0D20, 0x0DEC, 0x0EB8, 0x0FCA, 0x10DC},
      {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08B4, 0x09C6, 0x0AD8, 0x0BEA, 0x0D0E, 0x0E32, 0x0EFE, 0x0FCA, 0x10DC},
      {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x086E, 0x0980, 0x0A92, 0x0BA4, 0x0CB6, 0x0DC8, 0x0EEC, 0x1010, 0x10DC},
      {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x086E, 0x093A, 0x0A4C, 0x0B5E, 0x0C70, 0x0D82, 0x0E94, 0x0FB8, 0x10DC}
    };
    u8 vzone2pzoneTable[7][16] = {
      {0x0, 0x1, 0x2, 0x9, 0x8, 0x3, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA},
      {0x0, 0x1, 0x2, 0x3, 0xA, 0x9, 0x8, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB},
      {0x0, 0x1, 0x2, 0x3, 0x4, 0xB, 0xA, 0x9, 0x8, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC},
      {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0xC, 0xB, 0xA, 0x9, 0x8, 0x6, 0x7, 0xF, 0xE, 0xD},
      {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0xF, 0xE},
      {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0xF},
      {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8}
    };

    //check all blocks
    u32 vzone = 0;
    for (u32 lba : range(0x10DC)) {
      if (lba >= vzoneLbaTable[type][vzone]) vzone++;

      //skip those that can't be copied
      if (lba < rom_start_lba) continue;
      if (lba > rom_end_lba && lba < ram_start_lba) continue;
      if (lba > ram_end_lba) continue;

      u32 pzoneCalc = vzone2pzoneTable[type][vzone];
      u32 headCalc = (pzoneCalc > 7) ? 1 : 0;

      size += blockSizeTable[headCalc ? pzoneCalc - 7 : pzoneCalc];
    }

    if(input.size() == size) return true;
  }
  return false;
}

auto Nintendo64DD::repeatCheck(std::span<const u8> input, u32 repeat, u32 size) -> bool {
  for(u32 i : range(size)) {
    for(u32 j : range(repeat)) {
      if (input[i] != input[(j * size) + i]) return false;
    }
  }
  return true;
}

auto Nintendo64DD::createErrorTable(std::span<const u8> input) -> std::vector<u8> {
  //basic disk format check (further d64 check will be done later)
  b1 ndd = (input.size() == 0x3DEC800);
  b1 mame = (input.size() == 0x435B0C0);
  b1 d64 = (input.size() >= 0x4F08) && (input.size() < 0x3D79140);
  //if neither mame or ndd or d64 format, don't do anything
  if (!ndd && !mame && !d64) return {};

  input.begin();
  std::vector<u8> output;
  output.resize(1175*2*2, 0);   //1175 tracks * 2 blocks per track * 2 sides

  //perform basic system area check, check if the data repeats and validity
  output[12] = 0;
  u32 systemBlocks[4] = {0, 1, 8, 9};
  if(ndd || mame) {
    //ndd and mame format only
    for(u32 n : range(4)) {
      u32 systemOffset = systemBlocks[n]*0x4D08;
      output[systemBlocks[n]] = 1;

      //validity check
      if((input[systemOffset + 0x00] != 0xE8)             //region magic (jpn & usa)
      && (input[systemOffset + 0x00] != 0x22)) continue;
      if((input[systemOffset + 0x01] != 0x48)
      && (input[systemOffset + 0x01] != 0x63)) continue;
      if((input[systemOffset + 0x02] != 0xD3)
      && (input[systemOffset + 0x02] != 0xEE)) continue;
      if((input[systemOffset + 0x03] != 0x16)
      && (input[systemOffset + 0x03] != 0x56)) continue;

      if(input[systemOffset + 0x04] != 0x10) continue;  //format type
      if(input[systemOffset + 0x05] <  0x10) continue;  //disk type
      if(input[systemOffset + 0x05] >= 0x17) continue;
      if(input[systemOffset + 0x18] != 0xFF) continue;  //always 0xFF
      if(input[systemOffset + 0x19] != 0xFF) continue;
      if(input[systemOffset + 0x1A] != 0xFF) continue;
      if(input[systemOffset + 0x1B] != 0xFF) continue;
      if(input[systemOffset + 0x1C] != 0x80) continue;  //load address

      //repeat check
      std::span<const u8> block{input.data() + systemOffset, 0xE8 * 0x55};
      if(!repeatCheck(block, 0x55, 0xE8))   continue;

      output[systemBlocks[n]] = 0;

      //confirmed retail, inject error in block 12
      output[12] = 1;
    }

    //if retail info check is bad, check if it's a dev one
    if(output[12] == 0) {
      for(u32 n : range(4)) {
        u32 systemBlock = ndd ? (systemBlocks[n]+2)^1 : (systemBlocks[n]+2);
        u32 systemOffset = systemBlock * 0x4D08;
        output[systemBlocks[n]+2] = 1;

        //validity check
        if(input[systemOffset + 0x00] != 0x00) continue;  //region magic (dev)
        if(input[systemOffset + 0x01] != 0x00) continue;
        if(input[systemOffset + 0x02] != 0x00) continue;
        if(input[systemOffset + 0x03] != 0x00) continue;
        if(input[systemOffset + 0x04] != 0x10) continue;  //format type
        if(input[systemOffset + 0x05] <  0x10) continue;  //disk type
        if(input[systemOffset + 0x05] >= 0x17) continue;
        if(input[systemOffset + 0x18] != 0xFF) continue;  //always 0xFF
        if(input[systemOffset + 0x19] != 0xFF) continue;
        if(input[systemOffset + 0x1A] != 0xFF) continue;
        if(input[systemOffset + 0x1B] != 0xFF) continue;
        if(input[systemOffset + 0x1C] != 0x80) continue;  //load address

        //repeat check
        std::span<const u8> block{input.data() + systemOffset, 0xC0 * 0x55};
        if(!repeatCheck(block, 0x55, 0xC0))   continue;

        output[systemBlocks[n]+2] = 0;
      }
    }
  } else if(d64) {
    //d64 format only, assume development disk
    output[12] = 0;
    for(u32 n : range(4)) {
      output[systemBlocks[n]] = 1;
      output[systemBlocks[n]+2] = 0;
    }
    for(u32 n : range(0x200)) {
      if(n == 0x005) continue;
      if(n == 0x006) continue;
      if(n == 0x007) continue;
      if(n == 0x01C) continue;
      if(n == 0x01D) continue;
      if(n == 0x01E) continue;
      if(n == 0x01F) continue;
      if(n == 0x0E0) continue;
      if(n == 0x0E1) continue;
      if(n == 0x0E2) continue;
      if(n == 0x0E3) continue;
      if(n == 0x0E4) continue;
      if(n == 0x0E5) continue;
      if(n >= 0x100 && n < 0x120) continue;
      if(n == 0x1E8) continue;

      if(input[n] != 0x00) return {};
    }
    if(input[0x005] >= 0x07) return {};   //disk type
    if(input[0x006] >= 0x02) return {};   //load size
    if(input[0x01C] != 0x80) return {};   //load address
    if(input[0x01D] >= 0x80) return {};
    if(input[0x0E0] > 0x10) return {};    //rom lba end
    if(input[0x0E2] > 0x10 && input[0x0E2] < 0xFF) return {}; //ram lba start
    if(input[0x0E4] > 0x10 && input[0x0E4] < 0xFF) return {}; //ram lba end
  }

  //check disk id info
  u32 diskIdBlocks[2] = {14, 15};
  if(ndd || mame) {
    for(u32 n : range(2)) {
      u32 diskIdOffset = diskIdBlocks[n]*0x4D08;
      output[diskIdBlocks[n]] = 1;

      //repeat check
      std::span<const u8> block{input.data() + diskIdOffset, 0xE8 * 0x55};
      if(!repeatCheck(block, 0x55, 0xE8))   continue;

      output[diskIdBlocks[n]] = 0;
    }
  } else if(d64) {
    for(u32 n : range(2)) {
      output[diskIdBlocks[n]] = 0;
    }
  }

  return output;
}

auto Nintendo64DD::transform(std::span<const u8> input, std::vector<u8> errorTable) -> std::vector<u8> {
  //basic disk format check (further d64 check will be done later)
  b1 ndd = (input.size() == 0x3DEC800);
  b1 mame = (input.size() == 0x435B0C0);
  b1 d64 = (input.size() >= 0x4F08) && (input.size() < 0x3D79140);
  //if neither mame or ndd or d64 format, don't do anything
  if (!ndd && !mame && !d64) return {};

  //only recognize base retail ndd for now
  if(mame) {
    //mame physical format (canon ares format)
    //just copy
    std::vector<u8> output;
    output.resize(0x435B0C0, 0);

    for(u32 n : range(input.size()))
      output[n] = input[n];
    
    return output;
  }

  u32 systemOffset = 0;
  b1 dev = errorTable[12] == 0;
  if(ndd) {
    //ndd dump format (convert to mame format)

    //perform basic system area check
    b1 systemCheck = false;
    u32 systemBlocks[4] = {9, 8, 1, 0};
    for(u32 n : range(4)) {
      if(dev) {
        u32 systemBlock = systemBlocks[n]+2;
        if(errorTable[systemBlock] == 0) {
          systemCheck = true;
          systemOffset = (systemBlock^1)*0x4D08;
        }
      } else {
        u32 systemBlock = systemBlocks[n];
        if(errorTable[systemBlock] == 0) {
          systemCheck = true;
          systemOffset = systemBlock*0x4D08;
        }
      }
    }

    //check failed
    if(!systemCheck) return {};
  }

  //make sure to use valid disk info when converting
  u32 inputPos = 0;  // Track position in input
  std::span<const u8> dataFormat{input.data() + systemOffset, 0xE8};

  //ndd conv
  std::vector<u8> output;
  output.resize(0x435B0C0, 0);

  u32 lba = 0;
  u32 type = dataFormat[5] & 0xF;
  u32 blockSizeTable[9] = {0x4D08, 0x47B8, 0x4510, 0x3FC0, 0x3A70, 0x3520, 0x2FD0, 0x2A80, 0x2530};
  u16 vzoneLbaTable[7][16] = {
    {0x0124, 0x0248, 0x035A, 0x047E, 0x05A2, 0x06B4, 0x07C6, 0x08D8, 0x09EA, 0x0AB6, 0x0B82, 0x0C94, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
    {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x06A2, 0x07C6, 0x08D8, 0x09EA, 0x0AFC, 0x0BC8, 0x0C94, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
    {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08C6, 0x09EA, 0x0AFC, 0x0C0E, 0x0CDA, 0x0DA6, 0x0EB8, 0x0FCA, 0x10DC},
    {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08B4, 0x09C6, 0x0AEA, 0x0C0E, 0x0D20, 0x0DEC, 0x0EB8, 0x0FCA, 0x10DC},
    {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x08B4, 0x09C6, 0x0AD8, 0x0BEA, 0x0D0E, 0x0E32, 0x0EFE, 0x0FCA, 0x10DC},
    {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x086E, 0x0980, 0x0A92, 0x0BA4, 0x0CB6, 0x0DC8, 0x0EEC, 0x1010, 0x10DC},
    {0x0124, 0x0248, 0x035A, 0x046C, 0x057E, 0x0690, 0x07A2, 0x086E, 0x093A, 0x0A4C, 0x0B5E, 0x0C70, 0x0D82, 0x0E94, 0x0FB8, 0x10DC}
  };
  u8 vzone2pzoneTable[7][16] = {
    {0x0, 0x1, 0x2, 0x9, 0x8, 0x3, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA},
    {0x0, 0x1, 0x2, 0x3, 0xA, 0x9, 0x8, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB},
    {0x0, 0x1, 0x2, 0x3, 0x4, 0xB, 0xA, 0x9, 0x8, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC},
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0xC, 0xB, 0xA, 0x9, 0x8, 0x6, 0x7, 0xF, 0xE, 0xD},
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0xF, 0xE},
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0xF},
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8}
  };
  u16 trackPhysicalTable[] = {0x000, 0x09E, 0x13C, 0x1D1, 0x266, 0x2FB, 0x390, 0x425,
                              0x091, 0x12F, 0x1C4, 0x259, 0x2EE, 0x383, 0x418, 0x48A};

  if(ndd) {
    u32 vzone = 0;
    for (; lba < 0x10DC; lba++) {
      if (lba >= vzoneLbaTable[type][vzone]) vzone++;
      u32 pzoneCalc = vzone2pzoneTable[type][vzone];
      u32 headCalc = (pzoneCalc > 7) ? 1 : 0;

      u32 lba_vzone = lba;
      if (vzone > 0) lba_vzone -= vzoneLbaTable[type][vzone - 1];

      u32 trackStart = trackPhysicalTable[headCalc ? pzoneCalc - 8 : pzoneCalc];
      u32 trackCalc = trackPhysicalTable[pzoneCalc];
      if (headCalc) trackCalc -= (lba_vzone >> 1);
      else trackCalc += (lba_vzone >> 1);

      u32 defectOffset = 0;
      if (pzoneCalc > 0) defectOffset = dataFormat[8 + pzoneCalc - 1];
      u32 defectAmount = dataFormat[8 + pzoneCalc] - defectOffset;

      while ((defectAmount != 0) && ((dataFormat[0x20 + defectOffset] + trackStart) <= trackCalc)) {
        trackCalc++;
        defectOffset++;
        defectAmount--;
      }

      u32 blockCalc = ((lba & 3) == 0 || (lba & 3) == 3) ? 0 : 1;

      u32 startOffsetTable[16] = {0x0,0x5F15E0,0xB79D00,0x10801A0,0x1523720,0x1963D80,0x1D414C0,0x20BBCE0,
                                  0x23196E0,0x28A1E00,0x2DF5DC0,0x3299340,0x36D99A0,0x3AB70E0,0x3E31900,0x4149200};
      u32 offsetCalc = startOffsetTable[pzoneCalc];
      offsetCalc += (trackCalc - trackStart) * (blockSizeTable[headCalc ? pzoneCalc - 7 : pzoneCalc] * 2);
      offsetCalc += blockSizeTable[headCalc ? pzoneCalc - 7 : pzoneCalc] * blockCalc;

      for(u32 n : range(blockSizeTable[headCalc ? pzoneCalc - 7 : pzoneCalc]))
        output[offsetCalc + n] = input[inputPos++];
    }
  }
  if(d64) {
    //copy tons of system area stuff (we can cheat on this one)
    u32 systemBlocks[4] = {0, 1, 8, 9};
    for(u32 n : range(4)) {
      u32 offsetCalc = (systemBlocks[n] + (dev ? 2 : 0)) * blockSizeTable[0];
      //copy first sector of system data
      for(u32 m : range(dev ? 0xC0 : 0xE8)) {
        output[offsetCalc + m] = dataFormat[m];
      }
      //inject proper disk data
      output[offsetCalc + 0x004] = 0x10;  //format type
      output[offsetCalc + 0x005] += 0x10; //disk type
      //copy to all sectors
      for(u32 m : range(85)) {
        for(u32 l : range(dev ? 0xC0 : 0xE8)) {
          output[offsetCalc + (m * (dev ? 0xC0 : 0xE8)) + l] = output[offsetCalc + l];
        }
      }
    }
    //same for disk id
    u32 diskIdBlocks[2] = {14, 15};
    for(u32 n : range(2)) {
      u32 offsetCalc = diskIdBlocks[n] * blockSizeTable[0];
      //copy disk id to all sectors
      for(u32 m : range(85)) {
        for(u32 l : range(blockSizeTable[0] / 85)) {
          output[offsetCalc + (m * (blockSizeTable[0] / 85)) + l] = input[0x100 + l];
        }
      }
    }

    //get lba info
    u32 rom_start_lba = 24;
    u32 rom_end_lba = (input[0xE0] << 8) + input[0xE1] + 24;
    u32 ram_start_lba = (input[0xE2] << 8) + input[0xE3];
    u32 ram_end_lba = (input[0xE4] << 8) + input[0xE5];
    //either both are 0xFFFF or aren't
    if((ram_start_lba == 0xFFFF) && (ram_end_lba != 0xFFFF)) return {};
    if((ram_start_lba != 0xFFFF) && (ram_end_lba == 0xFFFF)) return {};
    //add base lba
    if((ram_start_lba != 0xFFFF) && (ram_end_lba != 0xFFFF)) {
      ram_start_lba += 24;
      ram_end_lba += 24;
    }

    //copy lbas
    input = input.subspan(0x200);
    u32 vzone = 0;
    for (; lba < 0x10DC; lba++) {
      if (lba >= vzoneLbaTable[type][vzone]) vzone++;

      //skip those that can't be copied
      if (lba < rom_start_lba) continue;
      if (lba > rom_end_lba && lba < ram_start_lba) continue;
      if (lba > ram_end_lba) continue;

      u32 pzoneCalc = vzone2pzoneTable[type][vzone];
      u32 headCalc = (pzoneCalc > 7) ? 1 : 0;

      u32 lba_vzone = lba;
      if (vzone > 0) lba_vzone -= vzoneLbaTable[type][vzone - 1];

      u32 trackStart = trackPhysicalTable[headCalc ? pzoneCalc - 8 : pzoneCalc];
      u32 trackCalc = trackPhysicalTable[pzoneCalc];
      if (headCalc) trackCalc -= (lba_vzone >> 1);
      else trackCalc += (lba_vzone >> 1);

      u32 defectOffset = 0;
      if (pzoneCalc > 0) defectOffset = dataFormat[8 + pzoneCalc - 1];
      u32 defectAmount = dataFormat[8 + pzoneCalc] - defectOffset;

      while ((defectAmount != 0) && ((dataFormat[0x20 + defectOffset] + trackStart) <= trackCalc)) {
        trackCalc++;
        defectOffset++;
        defectAmount--;
      }

      u32 blockCalc = ((lba & 3) == 0 || (lba & 3) == 3) ? 0 : 1;

      u32 startOffsetTable[16] = {0x0,0x5F15E0,0xB79D00,0x10801A0,0x1523720,0x1963D80,0x1D414C0,0x20BBCE0,
                                  0x23196E0,0x28A1E00,0x2DF5DC0,0x3299340,0x36D99A0,0x3AB70E0,0x3E31900,0x4149200};
      u32 offsetCalc = startOffsetTable[pzoneCalc];
      offsetCalc += (trackCalc - trackStart) * (blockSizeTable[headCalc ? pzoneCalc - 7 : pzoneCalc] * 2);
      offsetCalc += blockSizeTable[headCalc ? pzoneCalc - 7 : pzoneCalc] * blockCalc;

      for(u32 n : range(blockSizeTable[headCalc ? pzoneCalc - 7 : pzoneCalc]))
        output[offsetCalc + n] = input[inputPos++];
    }
  }

  //add new timestamp to differenciate every disk file for swapping
  {
    //get current time
    chrono::timeinfo timeinfo = chrono::local::timeinfo();

    u8 rng = random();
    u8 second = BCD::encode(min(59, timeinfo.second));
    u8 minute = BCD::encode(timeinfo.minute);
    u8 hour = BCD::encode(timeinfo.hour);
    u8 day = BCD::encode(timeinfo.day);
    u8 month = BCD::encode(1 + timeinfo.month);
    u8 yearlo = BCD::encode(timeinfo.year % 100);
    u8 yearhi = BCD::encode(timeinfo.year / 100);

    u32 diskIdBlocks[2] = {14, 15};
    for(u32 n : range(2)) {
      u32 offsetCalc = diskIdBlocks[n] * blockSizeTable[0];
      //change disk id to all sectors
      for(u32 m : range(85)) {
        //change manufacture line info (0x08 to 0x0F) (is normally a BCD number but officially "PTN64" was also used there)
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x0B] = rng;
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x0C] = 0x41;  //'A'
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x0D] = 0x52;  //'R'
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x0E] = 0x45;  //'E'
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x0F] = 0x53;  //'S'
        //change disk manufacture time (0x10 to 0x17) to now
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x11] = yearhi;
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x12] = yearlo;
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x13] = month;
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x14] = day;
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x15] = hour;
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x16] = minute;
        output[offsetCalc + (m * (blockSizeTable[0] / 85)) + 0x17] = second;
      }
    }
  }

  return output;
}
