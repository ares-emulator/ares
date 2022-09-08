struct Nintendo64DD : FloppyDisk {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto extensions() -> vector<string> override { return {"n64dd", "ndd"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom, vector<u8> errorTable) -> string;
  auto transform(array_view<u8> input, vector<u8> errorTable) -> vector<u8>;
  auto repeatCheck(array_view<u8> input, u32 repeat, u32 size) -> bool;
  auto createErrorTable(array_view<u8> input) -> vector<u8>;
};

auto Nintendo64DD::load(string location) -> bool {
  vector<u8> input;
  if(directory::exists(location)) {
    append(input, {location, "program.disk"});
  } else if(file::exists(location)) {
    input = FloppyDisk::read(location);
  }
  if(!input) return false;

  array_view<u8> view{input};
  auto errorTable = createErrorTable(view);
  if(!errorTable) return false;

  this->location = location;
  this->manifest = analyze(input, errorTable);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = shared_pointer{new vfs::directory};
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.disk.error", errorTable);

  if(auto output = transform(view, errorTable)) {
    pak->append("program.disk", output);
  }

  if(!pak) return false;

  Pak::load("program.disk", ".disk");

  return true;
}

auto Nintendo64DD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  Pak::save("program.disk", ".disk");

  return true;
}

auto Nintendo64DD::analyze(vector<u8>& rom, vector<u8> errorTable) -> string {
  bool dev = errorTable[12] == 0;
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

auto Nintendo64DD::repeatCheck(array_view<u8> input, u32 repeat, u32 size) -> bool {
  for(u32 i : range(size)) {
    for(u32 j : range(repeat)) {
      if (input[i] != input[(j * size) + i]) return false;
    }
  }
  return true;
}

auto Nintendo64DD::createErrorTable(array_view<u8> input) -> vector<u8> {
  //if neither mame or ndd format, don't do anything
  if ((input.size() != 0x435B0C0) && (input.size() != 0x3DEC800)) return {};

  input.begin();
  vector<u8> output;
  output.resize(1175*2*2, 0);   //1175 tracks * 2 blocks per track * 2 sides

  //perform basic system area check, check if the data repeats and validity
  u32 systemBlocks[4] = {0, 1, 8, 9};
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
    array_view<u8> block{input.data() + systemOffset, 0xE8 * 0x55};
    if (!repeatCheck(block, 0x55, 0xE8))   continue;

    output[systemBlocks[n]] = 0;

    //confirmed retail, inject error in block 12
    output[12] = 1;
  }

  //if retail info check is bad, check if it's a dev one
  if(output[12] == 0) {
    for(u32 n : range(4)) {
      u32 systemOffset = ((systemBlocks[n]+2) ^ (input.size() == 0x435B0C0) ? 1 : 0) * 0x4D08;
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
      array_view<u8> block{input.data() + systemOffset, 0xC0 * 0x55};
      if (!repeatCheck(block, 0x55, 0xC0))   continue;

      output[systemBlocks[n]+2] = 0;
    }
  }

  //check disk id info
  u32 diskIdBlocks[2] = {14, 15};
  for(u32 n : range(2)) {
    u32 diskIdOffset = diskIdBlocks[n]*0x4D08;
    output[diskIdBlocks[n]] = 1;

    //repeat check
    array_view<u8> block{input.data() + diskIdOffset, 0xE8 * 0x55};
    if (!repeatCheck(block, 0x55, 0xE8))   continue;

    output[diskIdBlocks[n]] = 0;
  }

  return output;
}

auto Nintendo64DD::transform(array_view<u8> input, vector<u8> errorTable) -> vector<u8> {
  //only recognize base retail ndd for now
  if(input.size() == 0x435B0C0) {
    //mame physical format (canon ares format)
    //just copy
    input.begin();
    vector<u8> output;
    output.resize(0x435B0C0, 0);

    for(u32 n : range(input.size()))
      output[n] = input.read();
    
    return output;
  }

  if(input.size() != 0x3DEC800) return {};
  //ndd dump format (convert to mame format)

  //perform basic system area check
  b1 systemCheck = false;
  u32 systemBlocks[4] = {9, 8, 1, 0};
  u32 systemOffset = 0;
  for(u32 n : range(4)) {
    if(errorTable[systemBlocks[n]] == 0) {
      systemCheck = true;
      systemOffset = systemBlocks[n]*0x4D08;
    }
  }

  //check failed
  if(!systemCheck) return {};

  //make sure to use valid disk info when converting
  array_view<u8> dataFormat{input.data() + systemOffset, 0xE8};

  //ndd conv
  input.begin();
  vector<u8> output;
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
      output[offsetCalc + n] = input.read();
  }

  return output;
}
