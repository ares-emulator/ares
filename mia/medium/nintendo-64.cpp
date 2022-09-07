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
  pak->setAttribute("id",     document["game/id"].string());
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("mempak", (bool)document["game/mempak"]);
  pak->setAttribute("rumble", (bool)document["game/rumble"]);
  pak->setAttribute("cic",    document["game/board/cic"].string());
  pak->setAttribute("dd",     (bool)document["game/dd"]);
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
  } else if((data[0] == 0x80 && data[1] == 0x37 && data[2] == 0x12 && data[3] == 0x40)
         || (data[0] == 0x80 && data[1] == 0x27 && data[2] == 0x07 && data[3] == 0x40)) {   //64DD IPL
    //big endian
  } else if((data[0] == 0x37 && data[1] == 0x80 && data[2] == 0x40 && data[3] == 0x12)
         || (data[0] == 0x27 && data[1] == 0x80 && data[2] == 0x40 && data[3] == 0x07)) {   //64DD IPL
    //byte-swapped
    for(u32 index = 0; index < data.size(); index += 2) {
      u8 d0 = data[index + 0];
      u8 d1 = data[index + 1];
      data[index + 0] = d1;
      data[index + 1] = d0;
    }
  } else if((data[0] == 0x40 && data[1] == 0x12 && data[2] == 0x37 && data[3] == 0x80)
         || (data[0] == 0x40 && data[1] == 0x07 && data[2] == 0x27 && data[3] == 0x80)) {   //64DD IPL
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
  //note: NTSC 6101 != PAL 7102; they use different bootcodes
  //note: NTSC 6102 == PAL 7101
  //note: NTSC 6104 / PAL 7104 was never officially used
  //note: Except for above cases, NTSC 610x == PAL 710x
  bool ntsc = region == "NTSC";
  string cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";  //fallback; most common
  u32 crc32 = Hash::CRC32({&data[0x40], 0x9c0}).value();
  if(crc32 == 0x1deb51a9) cic = "CIC-NUS-6101"; // Always NTSC (Star Fox 64)
  if(crc32 == 0xec8b1325) cic = "CIC-NUS-7102"; // Always PAL (Lylat Wars)
  if(crc32 == 0xc08e5bd6) cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";
  if(crc32 == 0x03b8376a) cic = ntsc ? "CIC-NUS-6103" : "CIC-NUS-7103";
  if(crc32 == 0xcf7f41dc) cic = ntsc ? "CIC-NUS-6105" : "CIC-NUS-7105";
  if(crc32 == 0xd1059c6a) cic = ntsc ? "CIC-NUS-6106" : "CIC-NUS-7106";
  if(crc32 == 0x0c965795) cic = "CIC-NUS-8303"; // 64DD Retail IPL (Japanese)
  if(crc32 == 0x10c68b18) cic = "CIC-NUS-8401"; // 64DD Development IPL (Japanese)
  if(crc32 == 0x8feba21e) cic = "CIC-NUS-DDUS"; // 64DD Retail IPL (North American, unreleased)

  //detect the save type based on the game ID
  u32 eeprom  = 0;      //512_B or 2_KiB
  u32 sram    = 0;      //32_KiB
  u32 flash   = 0;      //128_KiB

  //supported peripherals
  bool mempak = false;               //Controller Memory Pak
  bool rumble = false;               //Rumble Pak
  bool dd     = id.beginsWith("C");  //64DD

  //512B EEPROM
  if(id == "NTW") {eeprom = 512; mempak = true;}                         //64 de Hakken!! Tamagotchi
  if(id == "NHF") {eeprom = 512;}                                        //64 Hanafuda: Tenshi no Yakusoku
  if(id == "NOS") {eeprom = 512; mempak = true; rumble = true;}          //64 Oozumou
  if(id == "NTC") {eeprom = 512; rumble = true;}                         //64 Trump Collection
  if(id == "NER") {eeprom = 512; rumble = true;}                         //AeroFighters Assault
  if(id == "NAG") {eeprom = 512; mempak = true;}                         //AeroGauge
  if(id == "NAB") {eeprom = 512; mempak = true; rumble = true;}          //Air Boarder 64
  if(id == "NS3") {eeprom = 512; mempak = true;}                         //AI Shougi 3
  if(id == "NTN") {eeprom = 512;}                                        //All Star Tennis '99
  if(id == "NBN") {eeprom = 512; mempak = true;}                         //Bakuretsu Muteki Bangaioh
  if(id == "NBK") {eeprom = 512; rumble = true;}                         //Banjo-Kazooie
  if(id == "NFH") {eeprom = 512; rumble = true;}                         //Bass Hunter 64
  if(id == "NMU") {eeprom = 512; mempak = true; rumble = true;}          //Big Mountain 2000
  if(id == "NBC") {eeprom = 512; mempak = true;}                         //Blast Corps
  if(id == "NBH") {eeprom = 512; rumble = true;}                         //Body Harvest
  if(id == "NHA") {eeprom = 512; mempak = true;}                         //Bomberman 64: Arcade Edition (J)
  if(id == "NBM") {eeprom = 512; mempak = true;}                         //Bomberman 64
  if(id == "NBV") {eeprom = 512; mempak = true; rumble = true;}          //Bomberman 64: The Second Attack!
  if(id == "NBD") {eeprom = 512; rumble = true;}                         //Bomberman Hero
  if(id == "NCT") {eeprom = 512; rumble = true;}                         //Chameleon Twist
  if(id == "NCH") {eeprom = 512; rumble = true;}                         //Chopper Attack
  if(id == "NCG") {eeprom = 512; mempak = true; rumble = true;}          //Choro Q 64 II - Hacha Mecha Grand Prix Race (J)
  if(id == "NP2") {eeprom = 512; mempak = true; rumble = true;}          //Chou Kuukan Night Pro Yakyuu King 2 (J)
  if(id == "NXO") {eeprom = 512; rumble = true;}                         //Cruis'n Exotica
  if(id == "NCU") {eeprom = 512; mempak = true;}                         //Cruis'n USA
  if(id == "NCX") {eeprom = 512;}                                        //Custom Robo
  if(id == "NDY") {eeprom = 512; mempak = true; rumble = true;}          //Diddy Kong Racing
  if(id == "NDQ") {eeprom = 512; mempak = true;}                         //Disney's Donald Duck - Goin' Quackers [Quack Attack (E)]
  if(id == "NDR") {eeprom = 512;}                                        //Doraemon: Nobita to 3tsu no Seireiseki
  if(id == "NN6") {eeprom = 512;}                                        //Dr. Mario 64
  if(id == "NDU") {eeprom = 512; rumble = true;}                         //Duck Dodgers starring Daffy Duck
  if(id == "NJM") {eeprom = 512;}                                        //Earthworm Jim 3D
  if(id == "NFW") {eeprom = 512; rumble = true;}                         //F-1 World Grand Prix
  if(id == "NF2") {eeprom = 512; rumble = true;}                         //F-1 World Grand Prix II
  if(id == "NKA") {eeprom = 512; mempak = true; rumble = true;}          //Fighters Destiny
  if(id == "NFG") {eeprom = 512; mempak = true; rumble = true;}          //Fighter Destiny 2
  if(id == "NGL") {eeprom = 512; mempak = true; rumble = true;}          //Getter Love!!
  if(id == "NGV") {eeprom = 512;}                                        //Glover
  if(id == "NGE") {eeprom = 512; rumble = true;}                         //GoldenEye 007
  if(id == "NHP") {eeprom = 512;}                                        //Heiwa Pachinko World 64
  if(id == "NPG") {eeprom = 512; rumble = true;}                         //Hey You, Pikachu!
  if(id == "NIJ") {eeprom = 512; rumble = true;}                         //Indiana Jones and the Infernal Machine
  if(id == "NIC") {eeprom = 512; rumble = true;}                         //Indy Racing 2000
  if(id == "NFY") {eeprom = 512; mempak = true; rumble = true;}          //Kakutou Denshou: F-Cup Maniax
  if(id == "NKI") {eeprom = 512; mempak = true;}                         //Killer Instinct Gold
  if(id == "NLL") {eeprom = 512; rumble = true;}                         //Last Legion UX
  if(id == "NLR") {eeprom = 512; rumble = true;}                         //Lode Runner 3-D
  if(id == "NKT") {eeprom = 512; mempak = true;}                         //Mario Kart 64
  if(id == "CLB") {eeprom = 512; rumble = true;}                         //Mario Party (NTSC)
  if(id == "NLB") {eeprom = 512; rumble = true;}                         //Mario Party (PAL)
  if(id == "NMW") {eeprom = 512; rumble = true;}                         //Mario Party 2
  if(id == "NML") {eeprom = 512; rumble = true;}                         //Mickey's Speedway USA
  if(id == "NTM") {eeprom = 512;}                                        //Mischief Makers [Yuke Yuke!! Trouble Makers (J)]
  if(id == "NMI") {eeprom = 512; rumble = true;}                         //Mission: Impossible
  if(id == "NMG") {eeprom = 512; mempak = true; rumble = true;}          //Monaco Grand Prix [Racing Simulation 2 (G)]
  if(id == "NMO") {eeprom = 512;}                                        //Monopoly
  if(id == "NMS") {eeprom = 512; mempak = true;}                         //Morita Shougi 64
  if(id == "NMR") {eeprom = 512; mempak = true; rumble = true;}          //Multi-Racing Championship
  if(id == "NCR") {eeprom = 512; mempak = true;}                         //Penny Racers [Choro Q 64 (J)]
  if(id == "NEA") {eeprom = 512;}                                        //PGA European Tour
  if(id == "NPW") {eeprom = 512;}                                        //Pilotwings 64
  if(id == "NPY") {eeprom = 512; rumble = true;}                         //Puyo Puyo Sun 64
  if(id == "NPT") {eeprom = 512; rumble = true;}                         //Puyo Puyon Party
  if(id == "NRA") {eeprom = 512; mempak = true; rumble = true;}          //Rally '99 (J)
  if(id == "NWQ") {eeprom = 512; mempak = true; rumble = true;}          //Rally Challenge 2000
  if(id == "NSU") {eeprom = 512; rumble = true;}                         //Rocket: Robot on Wheels
  if(id == "NSN") {eeprom = 512; mempak = true; rumble = true;}          //Snow Speeder (J)
  if(id == "NK2") {eeprom = 512; rumble = true;}                         //Snowboard Kids 2 [Chou Snobow Kids (J)]
  if(id == "NSV") {eeprom = 512; rumble = true;}                         //Space Station Silicon Valley
  if(id == "NFX") {eeprom = 512; rumble = true;}                         //Lylat Wars (E)
  if(id == "NFP") {eeprom = 512; rumble = true;}                         //Star Fox 64 (U)
  if(id == "NS6") {eeprom = 512; rumble = true;}                         //Star Soldier: Vanishing Earth
  if(id == "NNA") {eeprom = 512; rumble = true;}                         //Star Wars Episode I: Battle for Naboo
  if(id == "NRS") {eeprom = 512; rumble = true;}                         //Star Wars: Rogue Squadron
  if(id == "NSW") {eeprom = 512;}                                        //Star Wars: Shadows of the Empire
  if(id == "NSC") {eeprom = 512;}                                        //Starshot: Space Circus Fever
  if(id == "NSA") {eeprom = 512; rumble = true;}                         //Sonic Wings Assault (J)
  if(id == "NB6") {eeprom = 512; mempak = true;}                         //Super B-Daman: Battle Phoenix 64
  if(id == "NSM") {eeprom = 512;}                                        //Super Mario 64
  if(id == "NSS") {eeprom = 512; rumble = true;}                         //Super Robot Spirits
  if(id == "NTX") {eeprom = 512; rumble = true;}                         //Taz Express
  if(id == "NT6") {eeprom = 512;}                                        //Tetris 64
  if(id == "NTP") {eeprom = 512;}                                        //Tetrisphere
  if(id == "NTJ") {eeprom = 512; rumble = true;}                         //Tom & Jerry in Fists of Fury
  if(id == "NRC") {eeprom = 512; rumble = true;}                         //Top Gear Overdrive
  if(id == "NTR") {eeprom = 512; mempak = true; rumble = true;}          //Top Gear Rally (J + E)
  if(id == "NTB") {eeprom = 512; rumble = true;}                         //Transformers: Beast Wars Metals 64
  if(id == "NGU") {eeprom = 512; rumble = true;}                         //Tsumi to Batsu: Hoshi no Keishousha (Sin and Punishment)
  if(id == "NIR") {eeprom = 512; rumble = true;}                         //Utchan Nanchan no Hono no Challenger: Denryuu Ira Ira Bou
  if(id == "NVL") {eeprom = 512; rumble = true;}                         //V-Rally Edition '99
  if(id == "NVY") {eeprom = 512; rumble = true;}                         //V-Rally Edition '99 (J)
  if(id == "NWR") {eeprom = 512; mempak = true;}                         //Wave Race 64: Kawasaki Jet Ski
  if(id == "NWC") {eeprom = 512; rumble = true;}                         //Wild Choppers
  if(id == "NAD") {eeprom = 512;}                                        //Worms Armageddon (U)
  if(id == "NWU") {eeprom = 512;}                                        //Worms Armageddon (E)
  if(id == "NYK") {eeprom = 512; rumble = true;}                         //Yakouchuu II: Satsujin Kouro
  if(id == "NMZ") {eeprom = 512;}                                        //Zool - Majou Tsukai Densetsu (J)

  //Special case for Japanese version of Wetrix and Dark Rift
  if(id == "NDK" && region_code == 'J') {eeprom = 512;}                  //Dark Rift aka Space Dynamites
  if(id == "NWT") { 
    if(region_code == 'J') {eeprom = 512;}                               //Wetrix (J)
    else {mempak = true;}                                                //Wetrix (U + E)
  }                  

  //2KB EEPROM
  if(id == "NB7") {eeprom = 2_KiB; rumble = true;}                       //Banjo-Tooie
  if(id == "NGT") {eeprom = 2_KiB; mempak = true; rumble = true;}        //City-Tour GP: Zen-Nihon GT Senshuken
  if(id == "NFU") {eeprom = 2_KiB; rumble = true;}                       //Conker's Bad Fur Day
  if(id == "NCW") {eeprom = 2_KiB; rumble = true;}                       //Cruis'n World
  if(id == "NCZ") {eeprom = 2_KiB; rumble = true;}                       //Custom Robo V2
  if(id == "ND6") {eeprom = 2_KiB; rumble = true;}                       //Densha de Go! 64
  if(id == "NDO") {eeprom = 2_KiB; rumble = true;}                       //Donkey Kong 64
  if(id == "ND2") {eeprom = 2_KiB; rumble = true;}                       //Doraemon 2: Nobita to Hikari no Shinden
  if(id == "N3D") {eeprom = 2_KiB; rumble = true;}                       //Doraemon 3: Nobita no Machi SOS!
  if(id == "NMX") {eeprom = 2_KiB; mempak = true; rumble = true;}        //Excitebike 64
  if(id == "NGC") {eeprom = 2_KiB; mempak = true; rumble = true;}        //GT 64: Championship Edition
  if(id == "NIM") {eeprom = 2_KiB;}                                      //Ide Yosuke no Mahjong Juku
  if(id == "NK4") {eeprom = 2_KiB; rumble = true;}                       //Kirby 64: The Crystal Shards
  if(id == "NNB") {eeprom = 2_KiB; mempak = true; rumble = true;}        //Kobe Bryant in NBA Courtside
  if(id == "NMV") {eeprom = 2_KiB; rumble = true;}                       //Mario Party 3
  if(id == "NM8") {eeprom = 2_KiB; rumble = true;}                       //Mario Tennis
  if(id == "NEV") {eeprom = 2_KiB; rumble = true;}                       //Neon Genesis Evangelion
  if(id == "NPP") {eeprom = 2_KiB; mempak = true;}                       //Parlor! Pro 64: Pachinko Jikki Simulation Game
  if(id == "NUB") {eeprom = 2_KiB; mempak = true;}                       //PD Ultraman Battle Collection 64
  if(id == "NPD") {eeprom = 2_KiB; mempak = true; rumble = true;}        //Perfect Dark
  if(id == "NRZ") {eeprom = 2_KiB; rumble = true;}                       //Ridge Racer 64
  if(id == "NR7") {eeprom = 2_KiB;}                                      //Robot Poncots 64: 7tsu no Umi no Caramel
  if(id == "NEP") {eeprom = 2_KiB; rumble = true;}                       //Star Wars Episode I: Racer
  if(id == "NYS") {eeprom = 2_KiB; rumble = true;}                       //Yoshi's Story

  //Special cases for Japanese versions of Castlevania
  if(id == "ND3") {
    if(region_code == 'J') {eeprom = 2_KiB; rumble = true;}              //Akumajou Dracula Mokushiroku (J)
    else {mempak = true;}                                                //Castlevania
  }
  if(id == "ND4") {
    if(region_code == 'J') {eeprom = 2_KiB; rumble = true;}              //Akumajou Dracula Mokushiroku Gaiden: Legend of Cornell (J)
    else {mempak = true;}                                                //Castlevania - Legacy of Darkness
  }

  //32KB SRAM
  if(id == "NTE") {sram = 32_KiB; rumble = true;}                        //1080 Snowboarding
  if(id == "NVB") {sram = 32_KiB; rumble = true;}                        //Bass Rush - ECOGEAR PowerWorm Championship (J)
  if(id == "CFZ") {sram = 32_KiB; rumble = true;}                        //F-Zero X (J)
  if(id == "NFZ") {sram = 32_KiB; rumble = true;}                        //F-Zero X (U + E)
  if(id == "NSI") {sram = 32_KiB; mempak = true;}                        //Fushigi no Dungeon: Fuurai no Shiren 2
  if(id == "NG6") {sram = 32_KiB; rumble = true;}                        //Ganmare Goemon: Dero Dero Douchuu Obake Tenkomori
  if(id == "N3H") {sram = 32_KiB; rumble = true;}                        //Ganbare! Nippon! Olympics 2000
  if(id == "NGP") {sram = 32_KiB; mempak = true;}                        //Goemon: Mononoke Sugoroku
  if(id == "NYW") {sram = 32_KiB;}                                       //Harvest Moon 64
  if(id == "NHY") {sram = 32_KiB; mempak = true; rumble = true;}         //Hybrid Heaven (J)
  if(id == "NIB") {sram = 32_KiB; rumble = true;}                        //Itoi Shigesato no Bass Tsuri No. 1 Kettei Ban!
  if(id == "NPS") {sram = 32_KiB; mempak = true; rumble = true;}         //Jikkyou J.League 1999: Perfect Striker 2
  if(id == "NPA") {sram = 32_KiB; mempak = true;}                        //Jikkyou Powerful Pro Yakyuu 2000
  if(id == "NP4") {sram = 32_KiB; mempak = true;}                        //Jikkyou Powerful Pro Yakyuu 4
  if(id == "NJ5") {sram = 32_KiB; mempak = true;}                        //Jikkyou Powerful Pro Yakyuu 5
  if(id == "NP6") {sram = 32_KiB; mempak = true;}                        //Jikkyou Powerful Pro Yakyuu 6
  if(id == "NPE") {sram = 32_KiB; mempak = true;}                        //Jikkyou Powerful Pro Yakyuu Basic Ban 2001
  if(id == "NJG") {sram = 32_KiB; rumble = true;}                        //Jinsei Game 64
  if(id == "CZL") {sram = 32_KiB; rumble = true;}                        //Legend of Zelda: Ocarina of Time [Zelda no Densetsu - Toki no Ocarina (J)]
  if(id == "NZL") {sram = 32_KiB; rumble = true;}                        //Legend of Zelda: Ocarina of Time (E)
  if(id == "NKG") {sram = 32_KiB; mempak = true; rumble = true;}         //Major League Baseball featuring Ken Griffey Jr.
  if(id == "NMF") {sram = 32_KiB; rumble = true;}                        //Mario Golf 64
  if(id == "NRI") {sram = 32_KiB; mempak = true;}                        //New Tetris, The
  if(id == "NUT") {sram = 32_KiB; mempak = true; rumble = true;}         //Nushi Zuri 64
  if(id == "NUM") {sram = 32_KiB; rumble = true;}                        //Nushi Zuri 64: Shiokaze ni Notte
  if(id == "NOB") {sram = 32_KiB;}                                       //Ogre Battle 64: Person of Lordly Caliber
  if(id == "CPS") {sram = 32_KiB;}                                       //Pocket Monsters Stadium (J)
  if(id == "NPM") {sram = 32_KiB; mempak = true;}                        //Premier Manager 64
  if(id == "NB5") {sram = 32_KiB; rumble = true;}                        //Resident Evil 2 (Japan) aka Biohazard 2
  if(id == "NRE") {sram = 32_KiB; rumble = true;}                        //Resident Evil 2
  if(id == "NAL") {sram = 32_KiB; rumble = true;}                        //Super Smash Bros. [Nintendo All-Star! Dairantou Smash Brothers (J)]
  if(id == "NT3") {sram = 32_KiB; mempak = true;}                        //Shin Nihon Pro Wrestling - Toukon Road 2 - The Next Generation (J)
  if(id == "NS4") {sram = 32_KiB; mempak = true;}                        //Super Robot Taisen 64  
  if(id == "NA2") {sram = 32_KiB; mempak = true; rumble = true;}         //Virtual Pro Wrestling 2
  if(id == "NVP") {sram = 32_KiB; mempak = true; rumble = true;}         //Virtual Pro Wrestling 64
  if(id == "NWL") {sram = 32_KiB; rumble = true;}                        //Waialae Country Club: True Golf Classics
  if(id == "NW2") {sram = 32_KiB; rumble = true;}                        //WCW-nWo Revenge
  if(id == "NWX") {sram = 32_KiB; mempak = true; rumble = true;}         //WWF WrestleMania 2000

  //Special case for first Japanese revisions of Kirby 64, overrides earlier entry
  if(id == "NK4" && region_code == 'J' && revision < 2) {
    eeprom = 0;
    sram = 32_KiB;
    rumble = true;
  }

  //96KB SRAM
  if(id == "CDZ") {sram = 96_KiB; rumble = true;}                        //Dezaemon 3D

  //128KB Flash
  if(id == "NCC") {flash = 128_KiB; rumble = true;}                      //Command & Conquer
  if(id == "NDA") {flash = 128_KiB; mempak = true;}                      //Derby Stallion 64
  if(id == "NAF") {flash = 128_KiB; mempak = true;}                      //Doubutsu no Mori
  if(id == "NJF") {flash = 128_KiB; rumble = true;}                      //Jet Force Gemini
  if(id == "NKJ") {flash = 128_KiB; rumble = true;}                      //Ken Griffey Jr.'s Slugfest
  if(id == "NZS") {flash = 128_KiB; rumble = true;}                      //Legend of Zelda: Majora's Mask [Zelda no Densetsu - Mujura no Kamen (J)]
  if(id == "NM6") {flash = 128_KiB; rumble = true;}                      //Mega Man 64
  if(id == "NCK") {flash = 128_KiB; rumble = true;}                      //NBA Courtside 2 featuring Kobe Bryant
  if(id == "NMQ") {flash = 128_KiB; rumble = true;}                      //Paper Mario
  if(id == "NPN") {flash = 128_KiB;}                                     //Pokemon Puzzle League
  if(id == "NPF") {flash = 128_KiB;}                                     //Pokemon Snap [Pocket Monsters Snap (J)]
  if(id == "NPO") {flash = 128_KiB;}                                     //Pokemon Stadium
  if(id == "CP2") {flash = 128_KiB;}                                     //Pocket Monsters Stadium 2 (J)
  if(id == "NP3") {flash = 128_KiB;}                                     //Pokemon Stadium 2 [Pocket Monsters Stadium - Kin Gin (J)]
  if(id == "NRH") {flash = 128_KiB; rumble = true;}                      //Rockman Dash (J)
  if(id == "NSQ") {flash = 128_KiB; rumble = true;}                      //StarCraft 64
  if(id == "NT9") {flash = 128_KiB;}                                     //Tigger's Honey Hunt
  if(id == "NW4") {flash = 128_KiB; mempak = true; rumble = true;}       //WWF No Mercy
  //unlicensed
  if(id == "NDP") {flash = 128_KiB;}                                     //Dinosaur Planet

  //Mempak
  if(id == "NO7") {mempak = true; rumble = true;}                        //The World Is Not Enough
  if(id == "NAY") {mempak = true;}                                       //Aidyn Chronicles - The First Mage
  if(id == "NBS") {mempak = true; rumble = true;}                        //All-Star Baseball '99
  if(id == "NBE") {mempak = true; rumble = true;}                        //All-Star Baseball 2000
  if(id == "NAS") {mempak = true; rumble = true;}                        //All-Star Baseball 2001
  if(id == "NAR") {mempak = true; rumble = true;}                        //Armorines - Project S.W.A.R.M.
  if(id == "NAC") {mempak = true; rumble = true;}                        //Army Men - Air Combat
  if(id == "NAM") {mempak = true; rumble = true;}                        //Army Men - Sarge's Heroes
  if(id == "N32") {mempak = true; rumble = true;}                        //Army Men - Sarge's Heroes 2
  if(id == "NAH") {mempak = true; rumble = true;}                        //Asteroids Hyper 64
  if(id == "NLC") {mempak = true; rumble = true;}                        //Automobili Lamborghini [Super Speed Race 64 (J)]
  if(id == "NBJ") {mempak = true;}                                       //Bakushou Jinsei 64 - Mezase! Resort Ou
  if(id == "NB4") {mempak = true; rumble = true;}                        //Bass Masters 2000
  if(id == "NBX") {mempak = true; rumble = true;}                        //Battletanx
  if(id == "NBQ") {mempak = true; rumble = true;}                        //Battletanx - Global Assault
  if(id == "NZO") {mempak = true; rumble = true;}                        //Battlezone - Rise of the Black Dogs
  if(id == "NNS") {mempak = true; rumble = true;}                        //Beetle Adventure Racing
  if(id == "NBB") {mempak = true; rumble = true;}                        //Beetle Adventure Racing (J)
  if(id == "NBF") {mempak = true; rumble = true;}                        //Bio F.R.E.A.K.S.
  if(id == "NBP") {mempak = true; rumble = true;}                        //Blues Brothers 2000
  if(id == "NYW") {mempak = true;}                                       //Bokujou Monogatari 2
  if(id == "NBO") {mempak = true;}                                       //Bottom of the 9th
  if(id == "NOW") {mempak = true;}                                       //Brunswick Circuit Pro Bowling
  if(id == "NBL") {mempak = true; rumble = true;}                        //Buck Bumble
  if(id == "NBY") {mempak = true; rumble = true;}                        //Bug's Life, A
  if(id == "NB3") {mempak = true; rumble = true;}                        //Bust-A-Move '99 [Bust-A-Move 3 DX (E)]
  if(id == "NBU") {mempak = true;}                                       //Bust-A-Move 2 - Arcade Edition
  if(id == "NCL") {mempak = true; rumble = true;}                        //California Speed
  if(id == "NCD") {mempak = true; rumble = true;}                        //Carmageddon 64
  if(id == "NTS") {mempak = true;}                                       //Centre Court Tennis
  if(id == "NV2") {mempak = true; rumble = true;}                        //Chameleon Twist 2
  if(id == "NPK") {mempak = true;}                                       //Chou Kuukan Night Pro Yakyuu King (J)
  if(id == "NT4") {mempak = true; rumble = true;}                        //CyberTiger
  if(id == "NDW") {mempak = true; rumble = true;}                        //Daikatana, John Romero's
  if(id == "NGA") {mempak = true; rumble = true;}                        //Deadly Arts [G.A.S.P!! Fighter's NEXTream (E-J)]
  if(id == "NDE") {mempak = true; rumble = true;}                        //Destruction Derby 64
  if(id == "NDQ") {mempak = true;}                                       //Disney's Donald Duck - Goin' Quackers [Quack Attack (E)]
  if(id == "NTA") {mempak = true; rumble = true;}                        //Disney's Tarzan
  if(id == "NDM") {mempak = true;}                                       //Doom 64
  if(id == "NDH") {mempak = true;}                                       //Duel Heroes
  if(id == "NDN") {mempak = true; rumble = true;}                        //Duke Nukem 64
  if(id == "NDZ") {mempak = true; rumble = true;}                        //Duke Nukem - Zero Hour
  if(id == "NWI") {mempak = true; rumble = true;}                        //ECW Hardcore Revolution
  if(id == "NST") {mempak = true;}                                       //Eikou no Saint Andrews
  if(id == "NET") {mempak = true;}                                       //Quest 64 [Eltale Monsters (J) Holy Magic Century (E)]
  if(id == "NEG") {mempak = true; rumble = true;}                        //Extreme-G
  if(id == "NG2") {mempak = true; rumble = true;}                        //Extreme-G XG2
  if(id == "NHG") {mempak = true;}                                       //F-1 Pole Position 64
  if(id == "NFR") {mempak = true; rumble = true;}                        //F-1 Racing Championship
  if(id == "N8I") {mempak = true;}                                       //FIFA - Road to World Cup 98
  if(id == "N9F") {mempak = true;}                                       //FIFA 99
  if(id == "N7I") {mempak = true;}                                       //FIFA Soccer 64
  if(id == "NFS") {mempak = true;}                                       //Famista 64
  if(id == "NFF") {mempak = true; rumble = true;}                        //Fighting Force 64
  if(id == "NFD") {mempak = true; rumble = true;}                        //Flying Dragon
  if(id == "NFO") {mempak = true; rumble = true;}                        //Forsaken 64
  if(id == "NF9") {mempak = true;}                                       //Fox Sports College Hoops '99
  if(id == "NG5") {mempak = true; rumble = true;}                        //Ganbare Goemon - Neo Momoyama Bakufu no Odori [Mystical Ninja Starring Goemon]
  if(id == "NGX") {mempak = true; rumble = true;}                        //Gauntlet Legends
  if(id == "NGD") {mempak = true; rumble = true;}                        //Gauntlet Legends (J)
  if(id == "NX3") {mempak = true; rumble = true;}                        //Gex 3 - Deep Cover Gecko
  if(id == "NX2") {mempak = true;}                                       //Gex 64 - Enter the Gecko
  if(id == "NGM") {mempak = true; rumble = true;}                        //Goemon's Great Adventure [Mystical Ninja 2 Starring Goemon]
  if(id == "NGN") {mempak = true;}                                       //Golden Nugget 64
  if(id == "NHS") {mempak = true;}                                       //Hamster Monogatari 64
  if(id == "NM9") {mempak = true;}                                       //Harukanaru Augusta Masters 98
  if(id == "NHC") {mempak = true; rumble = true;}                        //Hercules - The Legendary Journeys
  if(id == "NHX") {mempak = true;}                                       //Hexen
  if(id == "NHK") {mempak = true; rumble = true;}                        //Hiryuu no Ken Twin
  if(id == "NHW") {mempak = true; rumble = true;}                        //Hot Wheels Turbo Racing
  if(id == "NHG") {mempak = true;}                                       //Human Grand Prix - New Generation 
  if(id == "NHV") {mempak = true; rumble = true;}                        //Hybrid Heaven (U + E)
  if(id == "NHT") {mempak = true; rumble = true;}                        //Hydro Thunder
  if(id == "NWB") {mempak = true; rumble = true;}                        //Iggy's Reckin' Balls
  if(id == "NWS") {mempak = true;}                                       //International Superstar Soccer '98
  if(id == "NIS") {mempak = true; rumble = true;}                        //International Superstar Soccer 2000
  if(id == "NJP") {mempak = true;}                                       //International Superstar Soccer 64
  if(id == "NDS") {mempak = true;}                                       //J.League Dynamite Soccer 64
  if(id == "NJE") {mempak = true;}                                       //J.League Eleven Beat 1997
  if(id == "NLJ") {mempak = true;}                                       //J.League Live 64
  if(id == "NMA") {mempak = true;}                                       //Jangou Simulation Mahjong Do 64
  if(id == "NCO") {mempak = true; rumble = true;}                        //Jeremy McGrath Supercross 2000
  if(id == "NGS") {mempak = true;}                                       //Jikkyou G1 Stable
  if(id == "NJ3") {mempak = true;}                                       //Jikkyou World Soccer 3
  if(id == "N64") {mempak = true; rumble = true;}                        //Kira to Kaiketsu! 64 Tanteidan
  if(id == "NKK") {mempak = true; rumble = true;}                        //Knockout Kings 2000
  if(id == "NLG") {mempak = true; rumble = true;}                        //LEGO Racers
  if(id == "N8M") {mempak = true; rumble = true;}                        //Madden Football 64
  if(id == "NMD") {mempak = true; rumble = true;}                        //Madden Football 2000
  if(id == "NFL") {mempak = true; rumble = true;}                        //Madden Football 2001
  if(id == "N2M") {mempak = true; rumble = true;}                        //Madden Football 2002
  if(id == "N9M") {mempak = true; rumble = true;}                        //Madden Football '99
  if(id == "NMJ") {mempak = true;}                                       //Mahjong 64
  if(id == "NMM") {mempak = true;}                                       //Mahjong Master
  if(id == "NHM") {mempak = true; rumble = true;}                        //Mia Hamm Soccer 64
  if(id == "NWK") {mempak = true; rumble = true;}                        //Michael Owens WLS 2000 [World League Soccer 2000 (E) / Telefoot Soccer 2000 (F)]
  if(id == "NV3") {mempak = true; rumble = true;}                        //Micro Machines 64 Turbo
  if(id == "NAI") {mempak = true;}                                       //Midway's Greatest Arcade Hits Volume 1
  if(id == "NMB") {mempak = true; rumble = true;}                        //Mike Piazza's Strike Zone
  if(id == "NBR") {mempak = true; rumble = true;}                        //Milo's Astro Lanes
  if(id == "NM4") {mempak = true; rumble = true;}                        //Mortal Kombat 4
  if(id == "NMY") {mempak = true; rumble = true;}                        //Mortal Kombat Mythologies - Sub-Zero
  if(id == "NP9") {mempak = true; rumble = true;}                        //Ms. Pac-Man - Maze Madness
  if(id == "NH5") {mempak = true;}                                       //Nagano Winter Olympics '98 [Hyper Olympics in Nagano 64 (J)]
  if(id == "NNM") {mempak = true;}                                       //Namco Museum 64
  if(id == "N9C") {mempak = true; rumble = true;}                        //Nascar '99
  if(id == "NN2") {mempak = true; rumble = true;}                        //Nascar 2000
  if(id == "NXG") {mempak = true;}                                       //NBA Hangtime
  if(id == "NBA") {mempak = true; rumble = true;}                        //NBA In the Zone '98 [NBA Pro '98 (E)]
  if(id == "NB2") {mempak = true; rumble = true;}                        //NBA In the Zone '99 [NBA Pro '99 (E)]
  if(id == "NWZ") {mempak = true; rumble = true;}                        //NBA In the Zone 2000
  if(id == "NB9") {mempak = true;}                                       //NBA Jam '99
  if(id == "NJA") {mempak = true; rumble = true;}                        //NBA Jam 2000
  if(id == "N9B") {mempak = true; rumble = true;}                        //NBA Live '99
  if(id == "NNL") {mempak = true; rumble = true;}                        //NBA Live 2000
  if(id == "NSO") {mempak = true;}                                       //NBA Showtime - NBA on NBC
  if(id == "NBZ") {mempak = true; rumble = true;}                        //NFL Blitz
  if(id == "NSZ") {mempak = true; rumble = true;}                        //NFL Blitz - Special Edition
  if(id == "NBI") {mempak = true; rumble = true;}                        //NFL Blitz 2000
  if(id == "NFB") {mempak = true; rumble = true;}                        //NFL Blitz 2001
  if(id == "NQ8") {mempak = true; rumble = true;}                        //NFL Quarterback Club '98
  if(id == "NQ9") {mempak = true; rumble = true;}                        //NFL Quarterback Club '99
  if(id == "NQB") {mempak = true; rumble = true;}                        //NFL Quarterback Club 2000
  if(id == "NQC") {mempak = true; rumble = true;}                        //NFL Quarterback Club 2001
  if(id == "N9H") {mempak = true; rumble = true;}                        //NHL '99
  if(id == "NHO") {mempak = true; rumble = true;}                        //NHL Blades of Steel '99 [NHL Pro '99 (E)]
  if(id == "NHL") {mempak = true; rumble = true;}                        //NHL Breakaway '98
  if(id == "NH9") {mempak = true; rumble = true;}                        //NHL Breakaway '99
  if(id == "NNC") {mempak = true; rumble = true;}                        //Nightmare Creatures
  if(id == "NCE") {mempak = true; rumble = true;}                        //Nuclear Strike 64
  if(id == "NOF") {mempak = true; rumble = true;}                        //Offroad Challenge
  if(id == "NHN") {mempak = true;}                                       //Olympic Hockey Nagano '98
  if(id == "NOM") {mempak = true;}                                       //Onegai Monsters
  if(id == "NPC") {mempak = true;}                                       //Pachinko 365 Nichi (J)
  if(id == "NYP") {mempak = true; rumble = true;}                        //Paperboy
  if(id == "NPX") {mempak = true; rumble = true;}                        //Polaris SnoCross
  if(id == "NPL") {mempak = true;}                                       //Power League 64 (J)
  if(id == "NPU") {mempak = true;}                                       //Power Rangers - Lightspeed Rescue
  if(id == "NKM") {mempak = true;}                                       //Pro Mahjong Kiwame 64 (J)
  if(id == "NNR") {mempak = true;}                                       //Pro Mahjong Tsuwamono 64 - Jansou Battle ni Chousen (J)
  if(id == "NPB") {mempak = true; rumble = true;}                        //Puzzle Bobble 64 (J)
  if(id == "NKQ") {mempak = true; rumble = true;}                        //Quake 64
  if(id == "NQ2") {mempak = true; rumble = true;}                        //Quake 2
  if(id == "NKR") {mempak = true;}                                       //Rakuga Kids (E)
  if(id == "NRP") {mempak = true; rumble = true;}                        //Rampage - World Tour
  if(id == "NRP") {mempak = true; rumble = true;}                        //Rampage 2 - Universal Tour
  if(id == "NRT") {mempak = true;}                                       //Rat Attack
  if(id == "NY2") {mempak = true;}                                       //Rayman 2 - The Great Escape
  if(id == "NFQ") {mempak = true; rumble = true;}                        //Razor Freestyle Scooter
  if(id == "NRV") {mempak = true; rumble = true;}                        //Re-Volt
  if(id == "NRD") {mempak = true; rumble = true;}                        //Ready 2 Rumble Boxing
  if(id == "N22") {mempak = true; rumble = true;}                        //Ready 2 Rumble Boxing - Round 2
  if(id == "NRO") {mempak = true; rumble = true;}                        //Road Rash 64
  if(id == "NRR") {mempak = true; rumble = true;}                        //Roadster's Trophy
  if(id == "NRT") {mempak = true;}                                       //Robotron 64
  if(id == "NRK") {mempak = true;}                                       //Rugrats in Paris - The Movie
  if(id == "NR2") {mempak = true; rumble = true;}                        //Rush 2 - Extreme Racing USA
  if(id == "NCS") {mempak = true; rumble = true;}                        //S.C.A.R.S.
  if(id == "NDC") {mempak = true; rumble = true;}                        //SD Hiryuu no Ken Densetsu (J)
  if(id == "NSH") {mempak = true;}                                       //Saikyou Habu Shougi (J)
  if(id == "NSF") {mempak = true; rumble = true;}                        //San Francisco Rush - Extreme Racing
  if(id == "NRU") {mempak = true; rumble = true;}                        //San Francisco Rush 2049
  if(id == "NSY") {mempak = true;}                                       //Scooby-Doo! - Classic Creep Capers
  if(id == "NSD") {mempak = true; rumble = true;}                        //Shadow Man
  if(id == "NSG") {mempak = true;}                                       //Shadowgate 64 - Trials Of The Four Towers
  if(id == "NTO") {mempak = true;}                                       //Shin Nihon Pro Wrestling - Toukon Road - Brave Spirits (J)
  if(id == "NS2") {mempak = true;}                                       //Simcity 2000
  if(id == "NSK") {mempak = true; rumble = true;}                        //Snowboard Kids [Snobow Kids (J)]
  if(id == "NDT") {mempak = true; rumble = true;}                        //South Park
  if(id == "NPR") {mempak = true; rumble = true;}                        //South Park Rally
  if(id == "NIV") {mempak = true; rumble = true;}                        //Space Invaders
  if(id == "NSL") {mempak = true; rumble = true;}                        //Spider-Man
  if(id == "NR3") {mempak = true; rumble = true;}                        //Stunt Racer 64
  if(id == "NBW") {mempak = true; rumble = true;}                        //Super Bowling
  if(id == "NSX") {mempak = true; rumble = true;}                        //Supercross 2000
  if(id == "NSP") {mempak = true; rumble = true;}                        //Superman
  if(id == "NPZ") {mempak = true; rumble = true;}                        //Susume! Taisen Puzzle Dama Toukon! Marumata Chou (J)
  if(id == "NL2") {mempak = true; rumble = true;}                        //Top Gear Rally 2
  if(id == "NR6") {mempak = true; rumble = true;}                        //Tom Clancy's Rainbow Six
  if(id == "NTT") {mempak = true;}                                       //Tonic Trouble
  if(id == "NTF") {mempak = true; rumble = true;}                        //Tony Hawk's Pro Skater
  if(id == "NTQ") {mempak = true; rumble = true;}                        //Tony Hawk's Pro Skater 2
  if(id == "NT3") {mempak = true; rumble = true;}                        //Tony Hawk's Pro Skater 3
  if(id == "NGB") {mempak = true; rumble = true;}                        //Top Gear Hyper Bike
  if(id == "NGR") {mempak = true; rumble = true;}                        //Top Gear Rally (U)
  if(id == "NTH") {mempak = true; rumble = true;}                        //Toy Story 2 - Buzz Lightyear to the Rescue!
  if(id == "N3P") {mempak = true; rumble = true;}                        //Triple Play 2000
  if(id == "NTU") {mempak = true;}                                       //Turok: Dinosaur Hunter [Turok: Jiku Sinshi (J)]
  if(id == "NRW") {mempak = true; rumble = true;}                        //Turok: Rage Wars
  if(id == "NT2") {mempak = true; rumble = true;}                        //Turok 2 - Seeds of Evil [Violence Killer - Turok New Generation (J)]
  if(id == "NTK") {mempak = true; rumble = true;}                        //Turok 3 - Shadow of Oblivion
  if(id == "NSB") {mempak = true; rumble = true;}                        //Twisted Edge - Extreme Snowboarding [King Hill 64 - Extreme Snowboarding (J)]
  if(id == "NV8") {mempak = true; rumble = true;}                        //Vigilante 8
  if(id == "NVG") {mempak = true; rumble = true;}                        //Vigilante 8 - Second Offense
  if(id == "NVC") {mempak = true;}                                       //Virtual Chess 64
  if(id == "NVR") {mempak = true;}                                       //Virtual Pool 64
  if(id == "NWV") {mempak = true; rumble = true;}                        //WCW: Backstage Assault
  if(id == "NWM") {mempak = true; rumble = true;}                        //WCW: Mayhem
  if(id == "NW3") {mempak = true; rumble = true;}                        //WCW: Nitro
  if(id == "NWN") {mempak = true; rumble = true;}                        //WCW vs. nWo - World Tour
  if(id == "NWW") {mempak = true; rumble = true;}                        //WWF: War Zone
  if(id == "NTI") {mempak = true; rumble = true;}                        //WWF: Attitude
  if(id == "NWG") {mempak = true;}                                       //Wayne Gretzky's 3D Hockey
  if(id == "NW8") {mempak = true;}                                       //Wayne Gretzky's 3D Hockey '98
  if(id == "NWD") {mempak = true; rumble = true;}                        //Winback
  if(id == "NWP") {mempak = true; rumble = true;}                        //Wipeout 64
  if(id == "NJ2") {mempak = true;}                                       //Wonder Project J2 - Koruro no Mori no Jozet (J)
  if(id == "N8W") {mempak = true;}                                       //World Cup '98
  if(id == "NWO") {mempak = true; rumble = true;}                        //World Driver Championship
  if(id == "NXF") {mempak = true; rumble = true;}                        //Xena Warrior Princess - The Talisman of Fate

  //Special case for Shindou editions of Super Mario 64 & Wave Race 64
  if(id == "NSM" && region_code == 'J' && revision == 3) {rumble = true;} //Super Mario 64: Shindou Edition (Rev 3)
  if(id == "NWR" && region_code == 'J' && revision == 2) {rumble = true;} //Wave Race 64: Kawasaki Jet Ski Shindou Edition (Rev 2)

  //Rumble
  if(id == "NJQ") {rumble = true;}                                       //Batman - Return of the Joker
  if(id == "NCB") {rumble = true;}                                       //Charlie Blast's Territory
  if(id == "NDF") {rumble = true;}                                       //Dance Dance Revolution - Disney Dancing Museum
  if(id == "NKE") {rumble = true;}                                       //Knife Edge - Nose Gunner
  if(id == "NMT") {rumble = true;}                                       //Magical Tetris Challenge
  if(id == "NM3") {rumble = true;}                                       //Monster Truck Madness 64
  if(id == "NRG") {rumble = true;}                                       //Rugrats - Scavenger Hunt [Treasure Hunt(E)]
  if(id == "NOH") {rumble = true;}                                       //Transformers Beast Wars: Transmetals
  if(id == "NWF") {rumble = true;}                                       //Wheel of Fortune

  //Homebrew (libdragon / Everdrive special header flag)
  if(id[1] == 'E' && id[2] == 'D') {
    n8 config = data[0x3f];
    if(config.bit(4,7) == 1) {eeprom = 512;}
    if(config.bit(4,7) == 2) {eeprom = 2_KiB;}
    if(config.bit(4,7) == 3) {sram = 32_KiB;}
    //if(config.bit(4,7) == 4) {sram = 96_KiB;}   //banked SRAM, not supported yet
    if(config.bit(4,7) == 5) {flash = 128_KiB;}
    if(config.bit(4,7) == 6) {sram = 128_KiB;}
    rumble = true;
    mempak = true;
  }

  string s;
  s += "game\n";
  s +={"  name:     ", Medium::name(location), "\n"};
  s +={"  title:    ", Medium::name(location), "\n"};
  s +={"  region:   ", region, "\n"};
  s +={"  id:       ", id, region_code, "\n"};
  if(mempak)
  s += "  mempak\n";
  if(rumble)
  s += "  rumble\n";
  if(dd)
  s += "  dd\n";
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
