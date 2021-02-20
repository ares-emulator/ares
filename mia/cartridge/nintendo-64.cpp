struct Nintendo64 : Cartridge {
  auto name() -> string override { return "Nintendo 64"; }
  auto extensions() -> vector<string> override { return {"n64", "v64", "z64"}; }
  auto export(string location) -> vector<u8> override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto Nintendo64::export(string location) -> vector<u8> {
  vector<u8> data;
  append(data, {location, "program.rom"});
  return data;
}

auto Nintendo64::heuristics(vector<u8>& data, string location) -> string {
  if(data.size() < 0x1000) {
    //too small
    return {};
  } else if(data[0] == 0x80 && data[1] == 0x37 && data[2] == 0x12 && data[3] == 0x40) {
    //big endian
  } else if(data[0] == 0x37 && data[1] == 0x80 && data[2] == 0x40 && data[3] == 0x12) {
    //byte-swapped
    for(u32 index = 0; index < data.size(); index += 2) {
      u8 d0 = data[index + 0];
      u8 d1 = data[index + 1];
      data[index + 0] = d1;
      data[index + 1] = d0;
    }
  } else if(data[0] == 0x40 && data[1] == 0x12 && data[2] == 0x37 && data[3] == 0x80) {
    //little endian
    for(u32 index = 0; index < data.size(); index += 4) {
      u8 d0 = data[index + 0];
      u8 d1 = data[index + 1];
      u8 d2 = data[index + 2];
      u8 d3 = data[index + 3];
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

  string id;
  id.append((char)data[0x3c]);
  id.append((char)data[0x3d]);

  //detect the CIC used for a given gamepak based on checksumming its bootcode
  //note: NTSC 6101 = PAL 7102 and NTSC 6102 = PAL 7101 (IDs are swapped)
  //note: NTSC 6104 / PAL 7104 was never officially used
  bool ntsc = region == "NTSC";
  string cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";  //fallback; most common
  u32 crc32 = Hash::CRC32({&data[0x40], 0x9c0}).value();
  if(crc32 == 0x1deb51a9) cic = ntsc ? "CIC-NUS-6101" : "CIC-NUS-7102";
  if(crc32 == 0xc08e5bd6) cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";
  if(crc32 == 0x03b8376a) cic = ntsc ? "CIC-NUS-6103" : "CIC-NUS-7103";
  if(crc32 == 0xcf7f41dc) cic = ntsc ? "CIC-NUS-6105" : "CIC-NUS-7105";
  if(crc32 == 0xd1059c6a) cic = ntsc ? "CIC-NUS-6106" : "CIC-NUS-7106";

  //detect the save type based on the game ID
  u32 eeprom = 0;  //512_B or 2_KiB
  u32 sram   = 0;  //32_KiB
  u32 flash  = 0;  //128_KiB

  //512B EEPROM
  if(id == "ER") eeprom = 512;     //AeroFighters Assault
  if(id == "AB") eeprom = 512;     //Air Boarder 64
  if(id == "TN") eeprom = 512;     //All Star Tennis '99
  if(id == "BK") eeprom = 512;     //Banjo-Kazooie
  if(id == "FH") eeprom = 512;     //Bass Hunter 64
  if(id == "MU") eeprom = 512;     //Big Mountain 2000
  if(id == "BC") eeprom = 512;     //Blast Corps
  if(id == "BH") eeprom = 512;     //Body Harvest
  if(id == "BM") eeprom = 512;     //Bomberman 64
  if(id == "BV") eeprom = 512;     //Bomberman 64: The Second Attack!
  if(id == "BD") eeprom = 512;     //Bomberman Hero
  if(id == "CT") eeprom = 512;     //Chameleon Twist
  if(id == "CH") eeprom = 512;     //Chopper Attack
  if(id == "XO") eeprom = 512;     //Cruis'n Exotica
  if(id == "CU") eeprom = 512;     //Cruis'n USA
  if(id == "DY") eeprom = 512;     //Diddy Kong Racing
  if(id == "DQ") eeprom = 512;     //Donald Duck: Goin' Quackers
  if(id == "N6") eeprom = 512;     //Dr. Mario 64
  if(id == "JM") eeprom = 512;     //Earthworm Jim 3D
  if(id == "FW") eeprom = 512;     //F-1 World Grand Prix
  if(id == "F2") eeprom = 512;     //F-1 World Grand Prix II
  if(id == "KA") eeprom = 512;     //Fighters Destiny
  if(id == "FG") eeprom = 512;     //Fighter Destiny 2
  if(id == "GV") eeprom = 512;     //Glover
  if(id == "GE") eeprom = 512;     //GoldenEye 007
  if(id == "GC") eeprom = 512;     //GT 64: Championship Edition
  if(id == "PG") eeprom = 512;     //Hey You, Pikachu!
  if(id == "IJ") eeprom = 512;     //Indiana Jones and the Infernal Machine
  if(id == "IC") eeprom = 512;     //Indy Racing 2000
  if(id == "KI") eeprom = 512;     //Killer Instinct Gold
  if(id == "K4") eeprom = 512;     //Kirby 64: The Crystal Shards
  if(id == "LR") eeprom = 512;     //Lode Runner 3-D
  if(id == "DU") eeprom = 512;     //Duck Dodgers starring Daffy Duck
  if(id == "KT") eeprom = 512;     //Mario Kart 64
  if(id == "LB") eeprom = 512;     //Mario Party
  if(id == "MW") eeprom = 512;     //Mario Party 2
  if(id == "ML") eeprom = 512;     //Mickey's Speedway USA
  if(id == "TM") eeprom = 512;     //Mischief Makers
  if(id == "MI") eeprom = 512;     //Mission: Impossible
  if(id == "MO") eeprom = 512;     //Monopoly
  if(id == "MR") eeprom = 512;     //Multi-Racing Championship
  if(id == "CR") eeprom = 512;     //Penny Racers
  if(id == "EA") eeprom = 512;     //PGA European Tour
  if(id == "PW") eeprom = 512;     //Pilotwings 64
  if(id == "PM") eeprom = 512;     //Premier Manager 64
  if(id == "SU") eeprom = 512;     //Rocket: Robot on Wheels
  if(id == "K2") eeprom = 512;     //Snowboard Kids 2
  if(id == "SV") eeprom = 512;     //SpaceStation Silicon Valley
  if(id == "FX") eeprom = 512;     //Star Fox 64
  if(id == "S6") eeprom = 512;     //Star Soldier: Vanishing Earth
  if(id == "NA") eeprom = 512;     //Star Wars Episode I: Battle for Naboo
  if(id == "RS") eeprom = 512;     //Star Wars: Rogue Squadron
  if(id == "SW") eeprom = 512;     //Star Wars: Shadows of the Empire
  if(id == "SC") eeprom = 512;     //Starshot: Space Circus Fever
  if(id == "SM") eeprom = 512;     //Super Mario 64
  if(id == "TX") eeprom = 512;     //Tax Express
  if(id == "TP") eeprom = 512;     //Tetrisphere
  if(id == "TJ") eeprom = 512;     //Tom & Jerry in Fists of Fury
  if(id == "RC") eeprom = 512;     //Top Gear Overdrive
  if(id == "VL") eeprom = 512;     //V-Rally Edition '99
  if(id == "WL") eeprom = 512;     //Waialae Country Club: True Golf Classics
  if(id == "WR") eeprom = 512;     //Wave Race 64: Kawasaki Jet Ski
  if(id == "AD") eeprom = 512;     //Worms Armageddon

  //2KB EEPROM
  if(id == "B7") eeprom = 2_KiB;   //Banjo-Tooie
  if(id == "FU") eeprom = 2_KiB;   //Conker's Bad Fur Day
  if(id == "CW") eeprom = 2_KiB;   //Cruis'n World
  if(id == "DO") eeprom = 2_KiB;   //Donkey Kong 64
  if(id == "MX") eeprom = 2_KiB;   //Excitebike 64
  if(id == "NB") eeprom = 2_KiB;   //Kobe Bryant in NBA Courtside
  if(id == "MV") eeprom = 2_KiB;   //Mario Party 3
  if(id == "M8") eeprom = 2_KiB;   //Mario Tennis
  if(id == "PD") eeprom = 2_KiB;   //Perfect Dark
  if(id == "RZ") eeprom = 2_KiB;   //Ridge Racer 64
  if(id == "EP") eeprom = 2_KiB;   //Star Wars Episode I: Racer
  if(id == "YS") eeprom = 2_KiB;   //Yoshi's Story

  //32KB SRAM
  if(id == "TE") sram = 32_KiB;    //1080 Snowboarding
  if(id == "FZ") sram = 32_KiB;    //F-Zero X
  if(id == "YW") sram = 32_KiB;    //Harvest Moon 64
  if(id == "ZL") sram = 32_KiB;    //Legend of Zelda: The Ocarina of Time
  if(id == "KG") sram = 32_KiB;    //Major League Baseball featuring Ken Griffey Jr.
  if(id == "MF") sram = 32_KiB;    //Mario Golf 64
  if(id == "RI") sram = 32_KiB;    //New Tetris
  if(id == "OB") sram = 32_KiB;    //Ogre Battle 64: Person of Lordly Caliber
  if(id == "PS") sram = 32_KiB;    //Pokemon Stadium (JPN)
  if(id == "RE") sram = 32_KiB;    //Resident Evil 2
  if(id == "AL") sram = 32_KiB;    //Super Smash Bros.
  if(id == "W2") sram = 32_KiB;    //WCW/NWO Revenge
  if(id == "WX") sram = 32_KiB;    //WWF WrestleMania 2000

  //96KB SRAM
  if(id == "DZ") sram = 96_KiB;    //Dezaemon 3D

  //128KB Flash
  if(id == "CC") flash = 128_KiB;  //Command & Conquer
  if(id == "JF") flash = 128_KiB;  //Jet Force Gemini
  if(id == "KJ") flash = 128_KiB;  //Ken Griffey Jr.'s Slugfest
  if(id == "ZS") flash = 128_KiB;  //Legend of Zelda: Majora's Mask
  if(id == "M6") flash = 128_KiB;  //Mega Man 64
  if(id == "CK") flash = 128_KiB;  //NBA Courtside 2 featuring Kobe Bryant
  if(id == "MQ") flash = 128_KiB;  //Paper Mario
  if(id == "PN") flash = 128_KiB;  //Pokemon Puzzle League
  if(id == "PF") flash = 128_KiB;  //Pokemon Snap
  if(id == "PO") flash = 128_KiB;  //Pokemon Stadium
  if(id == "P3") flash = 128_KiB;  //Pokemon Stadium 2
  if(id == "SQ") flash = 128_KiB;  //StarCraft 64
  if(id == "T9") flash = 128_KiB;  //Tigger's Honey Hunt
  if(id == "W4") flash = 128_KiB;  //WWF No Mercy
  //unlicensed
  if(id == "DP") flash = 128_KiB;  //Dinosaur Planet

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  s +={"  id:     ", id, "\n"};
  s += "  board\n";
  s +={"    cic: ", cic, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  if(eeprom) {
  s += "    memory\n";
  s += "      type: EEPROM\n";
  s +={"      size: 0x", hex(eeprom), "\n"};
  s += "      content: Save\n";
  }
  if(sram) {
  s += "    memory\n";
  s += "      type: RAM\n";
  s +={"      size: 0x", hex(sram), "\n"};
  s += "      content: Save\n";
  }
  if(flash) {
  s += "    memory\n";
  s += "      type: Flash\n";
  s +={"      size: 0x", hex(flash), "\n"};
  s += "      content: Save\n";
  }
  return s;
}
