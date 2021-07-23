struct Nintendo64 : Cartridge {
  auto name() -> string override { return "Nintendo 64"; }
  auto extensions() -> vector<string> override { return {"n64", "v64", "z64"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto Nintendo64::load(string location) -> bool {
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
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("cic",    document["game/board/cic"].string());
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
  }

  return true;
}

auto Nintendo64::save(string location) -> bool {
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

auto Nintendo64::analyze(vector<u8>& data) -> string {
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
  case 'A': region = "NTSC"; break;  //North America + Japan
  case 'D': region = "PAL";  break;  //Germany
  case 'E': region = "NTSC"; break;  //North America
  case 'F': region = "PAL";  break;  //France
  case 'G': region = "NTSC"; break;  //Gateway 64 (NTSC)
  case 'I': region = "PAL";  break;  //Italy
  case 'J': region = "NTSC"; break;  //Japan
  case 'L': region = "PAL";  break;  //Gateway 64 (PAL)
  case 'P': region = "PAL";  break;  //Europe
  case 'S': region = "PAL";  break;  //Spain
  case 'U': region = "PAL";  break;  //Australia
  case 'X': region = "PAL";  break;  //Europe
  case 'Y': region = "PAL";  break;  //Europe
  }

  string id;
  id.append((char)data[0x3b]);
  id.append((char)data[0x3c]);
  id.append((char)data[0x3d]);

  char region_code = data[0x3e];
  u8 revision = data[0x3f];

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
  if(id == "NTW") eeprom = 512;     //64 de Hakken!! Tamagotchi
  if(id == "NHF") eeprom = 512;     //64 Hanafuda: Tenshi no Yakusoku
  if(id == "NOS") eeprom = 512;     //64 Oozumou
  if(id == "NTC") eeprom = 512;     //64 Trump Collection
  if(id == "NER") eeprom = 512;     //AeroFighters Assault (North America)
  if(id == "NSA") eeprom = 512;     //AeroFighters Assault (PAL, Japan)
  if(id == "NAG") eeprom = 512;     //AeroGauge
  if(id == "NAB") eeprom = 512;     //Air Boarder 64
  if(id == "NTN") eeprom = 512;     //All Star Tennis '99
  if(id == "NBN") eeprom = 512;     //Bakuretsu Muteki Bangaioh
  if(id == "NBK") eeprom = 512;     //Banjo-Kazooie
  if(id == "NFH") eeprom = 512;     //Bass Hunter 64
  if(id == "NMU") eeprom = 512;     //Big Mountain 2000
  if(id == "NBC") eeprom = 512;     //Blast Corps
  if(id == "NBH") eeprom = 512;     //Body Harvest
  if(id == "NHA") eeprom = 512;     //Bomber Man 64 (Japan)
  if(id == "NBM") eeprom = 512;     //Bomberman 64
  if(id == "NBV") eeprom = 512;     //Bomberman 64: The Second Attack!
  if(id == "NBD") eeprom = 512;     //Bomberman Hero
  if(id == "NCT") eeprom = 512;     //Chameleon Twist
  if(id == "NCH") eeprom = 512;     //Chopper Attack
  if(id == "NCG") eeprom = 512;     //Choro Q 64 2: Hacha-Mecha Grand Prix Race
  if(id == "NP2") eeprom = 512;     //Chou Kuukan Nighter Pro Yakyuu King 2
  if(id == "NXO") eeprom = 512;     //Cruis'n Exotica
  if(id == "NCU") eeprom = 512;     //Cruis'n USA
  if(id == "NCX") eeprom = 512;     //Custom Robo
  if(id == "NDY") eeprom = 512;     //Diddy Kong Racing
  if(id == "NDQ") eeprom = 512;     //Donald Duck: Goin' Quackers
  if(id == "NDR") eeprom = 512;     //Doraemon: Nobita to 3tsu no Seireiseki
  if(id == "NN6") eeprom = 512;     //Dr. Mario 64
  if(id == "NDU") eeprom = 512;     //Duck Dodgers starring Daffy Duck
  if(id == "NJM") eeprom = 512;     //Earthworm Jim 3D
  if(id == "NFW") eeprom = 512;     //F-1 World Grand Prix
  if(id == "NF2") eeprom = 512;     //F-1 World Grand Prix II
  if(id == "NKA") eeprom = 512;     //Fighters Destiny
  if(id == "NFG") eeprom = 512;     //Fighter Destiny 2
  if(id == "NGL") eeprom = 512;     //Getter Love!!
  if(id == "NGV") eeprom = 512;     //Glover
  if(id == "NGE") eeprom = 512;     //GoldenEye 007
  if(id == "NHP") eeprom = 512;     //Heiwa Pachinko World 64
  if(id == "NPG") eeprom = 512;     //Hey You, Pikachu!
  if(id == "NIJ") eeprom = 512;     //Indiana Jones and the Infernal Machine
  if(id == "NIC") eeprom = 512;     //Indy Racing 2000
  if(id == "NFY") eeprom = 512;     //Kakutou Denshou: F-Cup Maniax
  if(id == "NKI") eeprom = 512;     //Killer Instinct Gold
  if(id == "NLL") eeprom = 512;     //Last Legion UX
  if(id == "NLR") eeprom = 512;     //Lode Runner 3-D
  if(id == "NKT") eeprom = 512;     //Mario Kart 64
  if(id == "CLB") eeprom = 512;     //Mario Party (NTSC)
  if(id == "NLB") eeprom = 512;     //Mario Party (PAL)
  if(id == "NMW") eeprom = 512;     //Mario Party 2
  if(id == "NML") eeprom = 512;     //Mickey's Speedway USA
  if(id == "NTM") eeprom = 512;     //Mischief Makers
  if(id == "NMI") eeprom = 512;     //Mission: Impossible
  if(id == "NMG") eeprom = 512;     //Monaco Grand Prix
  if(id == "NMO") eeprom = 512;     //Monopoly
  if(id == "NMS") eeprom = 512;     //Morita Shougi 64
  if(id == "NMR") eeprom = 512;     //Multi-Racing Championship
  if(id == "NCR") eeprom = 512;     //Penny Racers
  if(id == "NEA") eeprom = 512;     //PGA European Tour
  if(id == "NPW") eeprom = 512;     //Pilotwings 64
  if(id == "NPM") eeprom = 512;     //Premier Manager 64
  if(id == "NPY") eeprom = 512;     //Puyo Puyo Sun 64
  if(id == "NPT") eeprom = 512;     //Puyo Puyon Party
  if(id == "NRA") eeprom = 512;     //Rally '99
  if(id == "NWQ") eeprom = 512;     //Rally Challenge 2000
  if(id == "NSU") eeprom = 512;     //Rocket: Robot on Wheels
  if(id == "NSN") eeprom = 512;     //Snow Speeder
  if(id == "NK2") eeprom = 512;     //Snowboard Kids 2
  if(id == "NSV") eeprom = 512;     //Space Station Silicon Valley
  if(id == "NFX") eeprom = 512;     //Star Fox 64
  if(id == "NS6") eeprom = 512;     //Star Soldier: Vanishing Earth
  if(id == "NNA") eeprom = 512;     //Star Wars Episode I: Battle for Naboo
  if(id == "NRS") eeprom = 512;     //Star Wars: Rogue Squadron
  if(id == "NSW") eeprom = 512;     //Star Wars: Shadows of the Empire
  if(id == "NSC") eeprom = 512;     //Starshot: Space Circus Fever
  if(id == "NB6") eeprom = 512;     //Super B-Daman: Battle Phoenix 64
  if(id == "NSM") eeprom = 512;     //Super Mario 64
  if(id == "NSS") eeprom = 512;     //Super Robot Spirits
  if(id == "NTX") eeprom = 512;     //Taz Express
  if(id == "NT6") eeprom = 512;     //Tetris 64
  if(id == "NTP") eeprom = 512;     //Tetrisphere
  if(id == "NTJ") eeprom = 512;     //Tom & Jerry in Fists of Fury
  if(id == "NRC") eeprom = 512;     //Top Gear Overdrive
  if(id == "NTR") eeprom = 512;     //Top Gear Rally (Japan, PAL)
  if(id == "NTB") eeprom = 512;     //Transformers: Beast Wars Metals 64
  if(id == "NGU") eeprom = 512;     //Tsumi to Batsu: Hoshi no Keishousha
  if(id == "NIR") eeprom = 512;     //Utchan Nanchan no Hono no Challenger: Denryuu Ira Ira Bou
  if(id == "NVL") eeprom = 512;     //V-Rally Edition '99 (North America, PAL)
  if(id == "NVY") eeprom = 512;     //V-Rally Edition '99 (Japan)
  if(id == "NWR") eeprom = 512;     //Wave Race 64: Kawasaki Jet Ski
  if(id == "NWC") eeprom = 512;     //Wild Choppers
  if(id == "NAD") eeprom = 512;     //Worms Armageddon (NTSC)
  if(id == "NWU") eeprom = 512;     //Worms Armageddon (PAL)
  if(id == "NYK") eeprom = 512;     //Yakouchuu II: Satsujin Kouro

  //Special case for Japanese version of Wetrix and Dark Rift
  if(id == "NDK" && region_code == 'J') eeprom = 512; //Dark Rift aka Space Dynamites
  if(id == "NWT" && region_code == 'J') eeprom = 512; //Wetrix

  //2KB EEPROM
  if(id == "NB7") eeprom = 2_KiB;   //Banjo-Tooie
  if(id == "NGT") eeprom = 2_KiB;   //City-Tour GP: Zen-Nihon GT Senshuken
  if(id == "NFU") eeprom = 2_KiB;   //Conker's Bad Fur Day
  if(id == "NCW") eeprom = 2_KiB;   //Cruis'n World
  if(id == "NCZ") eeprom = 2_KiB;   //Custom Robo V2
  if(id == "ND6") eeprom = 2_KiB;   //Densha de Go! 64
  if(id == "NDO") eeprom = 2_KiB;   //Donkey Kong 64
  if(id == "ND2") eeprom = 2_KiB;   //Doraemon 2: Nobita to Hikari no Shinden
  if(id == "N3D") eeprom = 2_KiB;   //Doraemon 3: Nobita no Machi SOS!
  if(id == "NMX") eeprom = 2_KiB;   //Excitebike 64
  if(id == "NGC") eeprom = 2_KiB;   //GT 64: Championship Edition
  if(id == "NIM") eeprom = 2_KiB;   //Ide Yosuke no Mahjong Juku
  if(id == "NK4") eeprom = 2_KiB;   //Kirby 64: The Crystal Shards
  if(id == "NNB") eeprom = 2_KiB;   //Kobe Bryant in NBA Courtside
  if(id == "NMV") eeprom = 2_KiB;   //Mario Party 3
  if(id == "NM8") eeprom = 2_KiB;   //Mario Tennis
  if(id == "NEV") eeprom = 2_KiB;   //Neon Genesis Evangelion
  if(id == "NPP") eeprom = 2_KiB;   //Parlor! Pro 64: Pachinko Jikki Simulation Game
  if(id == "NUB") eeprom = 2_KiB;   //PD Ultraman Battle Collection 64
  if(id == "NPD") eeprom = 2_KiB;   //Perfect Dark
  if(id == "NRZ") eeprom = 2_KiB;   //Ridge Racer 64
  if(id == "NR7") eeprom = 2_KiB;   //Robot Poncots 64: 7tsu no Umi no Caramel
  if(id == "NEP") eeprom = 2_KiB;   //Star Wars Episode I: Racer
  if(id == "NYS") eeprom = 2_KiB;   //Yoshi's Story

  //Special cases for Japanese versions of Castlevania
  if(id == "ND3" && region_code == 'J') eeprom = 2_KiB; //Akumajou Dracula Mokushiroku
  if(id == "ND4" && region_code == 'J') eeprom = 2_KiB; //Akumajou Dracula Mokushiroku Gaiden: Legend of Cornell

  //32KB SRAM
  if(id == "NTE") sram = 32_KiB;    //1080 Snowboarding
  if(id == "NVB") sram = 32_KiB;    //Bass Rush: ECOGEAR PowerWorm Championship
  if(id == "CFZ") sram = 32_KiB;    //F-Zero X (NTSC)
  if(id == "NFZ") sram = 32_KiB;    //F-Zero X (PAL)
  if(id == "NSI") sram = 32_KiB;    //Fushigi no Dungeon: Fuurai no Shiren 2
  if(id == "NG6") sram = 32_KiB;    //Ganmare Goemon: Dero Dero Douchuu Obake Tenkomori
  if(id == "N3H") sram = 32_KiB;    //Ganbare! Nippon! Olympics 2000
  if(id == "NGP") sram = 32_KiB;    //Goemon: Mononoke Sugoroku
  if(id == "NYW") sram = 32_KiB;    //Harvest Moon 64
  if(id == "NHY") sram = 32_KiB;    //Hybrid Heaven (Japan)
  if(id == "NIB") sram = 32_KiB;    //Itoi Shigesato no Bass Tsuri No. 1 Kettei Ban!
  if(id == "NPS") sram = 32_KiB;    //Jikkyou J.League 1999: Perfect Striker 2
  if(id == "NPA") sram = 32_KiB;    //Jikkyou Powerful Pro Yakyuu 2000
  if(id == "NJ5") sram = 32_KiB;    //Jikkyou Powerful Pro Yakyuu 5
  if(id == "NP6") sram = 32_KiB;    //Jikkyou Powerful Pro Yakyuu 6
  if(id == "NPE") sram = 32_KiB;    //Jikkyou Powerful Pro Yakyuu Basic Ban 2001
  if(id == "NJG") sram = 32_KiB;    //Jinsei Game 64
  if(id == "CZL") sram = 32_KiB;    //Legend of Zelda: Ocarina of Time (NTSC)
  if(id == "NZL") sram = 32_KiB;    //Legend of Zelda: Ocarina of Time (PAL)
  if(id == "NKG") sram = 32_KiB;    //Major League Baseball featuring Ken Griffey Jr.
  if(id == "NMF") sram = 32_KiB;    //Mario Golf 64
  if(id == "NRI") sram = 32_KiB;    //New Tetris
  if(id == "NUT") sram = 32_KiB;    //Nushi Zuri 64
  if(id == "NUM") sram = 32_KiB;    //Nushi Zuri 64: Shiokaze ni Notte
  if(id == "NOB") sram = 32_KiB;    //Ogre Battle 64: Person of Lordly Caliber
  if(id == "CPS") sram = 32_KiB;    //Pokemon Stadium (Japan)
  if(id == "NB5") sram = 32_KiB;    //Resident Evil 2 (Japan) aka Biohazard 2
  if(id == "NRE") sram = 32_KiB;    //Resident Evil 2
  if(id == "NS4") sram = 32_KiB;    //Super Robot Taisen 64
  if(id == "NAL") sram = 32_KiB;    //Super Smash Bros.
  if(id == "NT3") sram = 32_KiB;    //Toukon Road 2
  if(id == "NA2") sram = 32_KiB;    //Virtual Pro Wrestling 2
  if(id == "NVP") sram = 32_KiB;    //Virtual Pro Wrestling 64
  if(id == "NWL") sram = 32_KiB;    //Waialae Country Club: True Golf Classics
  if(id == "NW2") sram = 32_KiB;    //WCW/NWO Revenge
  if(id == "NWX") sram = 32_KiB;    //WWF WrestleMania 2000

  //96KB SRAM
  if(id == "CDZ") sram = 96_KiB;    //Dezaemon 3D

  //128KB Flash
  if(id == "NCC") flash = 128_KiB;  //Command & Conquer
  if(id == "NDA") flash = 128_KiB;  //Derby Stallion 64
  if(id == "NAF") flash = 128_KiB;  //Doubutsu no Mori
  if(id == "NJF") flash = 128_KiB;  //Jet Force Gemini
  if(id == "NKJ") flash = 128_KiB;  //Ken Griffey Jr.'s Slugfest
  if(id == "NZS") flash = 128_KiB;  //Legend of Zelda: Majora's Mask
  if(id == "NM6") flash = 128_KiB;  //Mega Man 64
  if(id == "NCK") flash = 128_KiB;  //NBA Courtside 2 featuring Kobe Bryant
  if(id == "NMQ") flash = 128_KiB;  //Paper Mario
  if(id == "NPN") flash = 128_KiB;  //Pokemon Puzzle League
  if(id == "NPF") flash = 128_KiB;  //Pokemon Snap
  if(id == "NPO") flash = 128_KiB;  //Pokemon Stadium
  if(id == "CP2") flash = 128_KiB;  //Pokemon Stadium 2 (Japan)
  if(id == "NP3") flash = 128_KiB;  //Pokemon Stadium 2
  if(id == "NRH") flash = 128_KiB;  //Rockman Dash
  if(id == "NSQ") flash = 128_KiB;  //StarCraft 64
  if(id == "NT9") flash = 128_KiB;  //Tigger's Honey Hunt
  if(id == "NW4") flash = 128_KiB;  //WWF No Mercy
  //unlicensed
  if(id == "NDP") flash = 128_KiB;  //Dinosaur Planet

  //Special case for first Japanese revisions of Kirby 64, overrides earlier entry
  if(id == "NK4" && region_code == 'J' && revision < 2) {
    eeprom = 0;
    flash = 128_KiB;
  }

  string s;
  s += "game\n";
  s +={"  name:     ", Medium::name(location), "\n"};
  s +={"  title:    ", Medium::name(location), "\n"};
  s +={"  region:   ", region, "\n"};
  s +={"  id:       ", id, region_code, "\n"};
  if(revision < 4) {
  s +={"  revision: 1.", revision, "\n"};
  }
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
