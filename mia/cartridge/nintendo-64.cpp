struct Nintendo64 : Cartridge {
  auto name() -> string override { return "Nintendo 64"; }
  auto extensions() -> vector<string> override { return {"n64", "v64", "z64"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto Nintendo64::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto Nintendo64::heuristics(vector<uint8_t>& data, string location) -> string {
  if(data.size() < 0x1000) {
    //too small
    return {};
  } else if(data[0] == 0x80 && data[1] == 0x37 && data[2] == 0x12 && data[3] == 0x40) {
    //big endian
  } else if(data[0] == 0x37 && data[1] == 0x80 && data[2] == 0x40 && data[3] == 0x12) {
    //byte-swapped
    for(uint index = 0; index < data.size(); index += 2) {
      uint8_t d0 = data[index + 0];
      uint8_t d1 = data[index + 1];
      data[index + 0] = d1;
      data[index + 1] = d0;
    }
  } else if(data[0] == 0x40 && data[1] == 0x12 && data[2] == 0x37 && data[3] == 0x80) {
    //little endian
    for(uint index = 0; index < data.size(); index += 4) {
      uint8_t d0 = data[index + 0];
      uint8_t d1 = data[index + 1];
      uint8_t d2 = data[index + 2];
      uint8_t d3 = data[index + 3];
      data[index + 0] = d3;
      data[index + 1] = d2;
      data[index + 2] = d1;
      data[index + 3] = d0;
    }
  } else {
    //unrecognized
    return {};
  }

  string region = "NTSC";
  switch(data[0x3e]) {
  case 'A': region = "NTSC"; break;  //Asia
  case 'B': region = "NTSC"; break;  //Brazil
  case 'C': region = "NTSC"; break;  //China
  case 'D': region = "PAL";  break;  //Germany
  case 'E': region = "NTSC"; break;  //North America
  case 'F': region = "PAL";  break;  //France
  case 'G': region = "NTSC"; break;  //Gateway 64 (NTSC)
  case 'H': region = "PAL";  break;  //Netherlands
  case 'I': region = "PAL";  break;  //Italy
  case 'J': region = "NTSC"; break;  //Japan
  case 'K': region = "NTSC"; break;  //Korea
  case 'L': region = "PAL";  break;  //Gateway 64 (PAL)
  case 'N': region = "NTSC"; break;  //Canada
  case 'P': region = "PAL";  break;  //Europe
  case 'S': region = "PAL";  break;  //Spain
  case 'U': region = "PAL";  break;  //Australia
  case 'W': region = "PAL";  break;  //Scandanavia
  case 'X': region = "PAL";  break;  //Europe
  case 'Y': region = "PAL";  break;  //Europe
  }

  //detect the CIC used for a given gamepak based on checksumming its bootcode
  //note: NTSC 6101 = PAL 7102 and NTSC 6102 = PAL 7101 (IDs are swapped)
  //note: NTSC 6104 / PAL 7104 was never officially used
  bool ntsc = region == "NTSC";
  string cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";  //fallback; most common
  uint32_t crc32 = Hash::CRC32({&data[0x40], 0x9c0}).value();
  if(crc32 == 0x1deb51a9) cic = ntsc ? "CIC-NUS-6101" : "CIC-NUS-7102";
  if(crc32 == 0xc08e5bd6) cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";
  if(crc32 == 0x03b8376a) cic = ntsc ? "CIC-NUS-6103" : "CIC-NUS-7103";
  if(crc32 == 0xcf7f41dc) cic = ntsc ? "CIC-NUS-6105" : "CIC-NUS-7105";
  if(crc32 == 0xd1059c6a) cic = ntsc ? "CIC-NUS-6106" : "CIC-NUS-7106";

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  s += "  board\n";
  s +={"    cic: ", cic, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
