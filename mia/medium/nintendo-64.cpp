struct Nintendo64 : Cartridge {
  auto name() -> string override { return "Nintendo 64"; }
  auto extensions() -> vector<string> override { return {"n64", "v64", "z64"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
  auto ipl2checksum(u32 seed, array_view<u8> rom) -> u64;
};

auto Nintendo64::ipl2checksum(u32 seed, array_view<u8> rom) -> u64 {
  auto rotl = [](u32 value, u32 shift) -> u32 {
    return (value << shift) | (value >> (-shift&31));
  };
  auto rotr = [](u32 value, u32 shift) -> u32 {
    return (value >> shift) | (value << (-shift&31));
  };

  auto csum = [](u32 a0, u32 a1, u32 a2) -> u32 {
    if (a1 == 0) a1 = a2;
    u64 prod = (u64)a0 * (u64)a1;
    u32 hi = (u32)(prod >> 32);
    u32 lo = (u32)prod;
    u32 diff = hi - lo;
    return diff ? diff : a0;
  };

  // create the initialization data
  u32 init = 0x6c078965 * (seed & 0xff) + 1;
  u32 data = rom.readm(4);
  init ^= data;

  // copy to the state
  u32 state[16];
  for(auto &s : state) s = init;

  u32 dataNext = data, dataLast;
  u32 loop = 0;
  while(1) {
      loop++;
      dataLast = data;
      data = dataNext;

      state[0] += csum(1007 - loop, data, loop);
      state[1]  = csum(state[1], data, loop);
      state[2] ^= data;
      state[3] += csum(data + 5, 0x6c078965, loop);
      state[9]  = (dataLast < data) ? csum(state[9], data, loop) : state[9] + data;
      state[4] += rotr(data, dataLast & 0x1f);
      state[7]  = csum(state[7], rotl(data, dataLast & 0x1f), loop);
      state[6]  = (data < state[6]) ? (state[3] + state[6]) ^ (data + loop) : (state[4] + data) ^ state[6];
      state[5] += rotl(data, dataLast >> 27);
      state[8]  = csum(state[8], rotr(data, dataLast >> 27), loop);

      if (loop == 1008) break;

      dataNext   = rom.readm(4);
      state[15]  = csum(csum(state[15], rotl(data, dataLast  >> 27), loop), rotl(dataNext, data  >> 27), loop);
      state[14]  = csum(csum(state[14], rotr(data, dataLast & 0x1f), loop), rotr(dataNext, data & 0x1f), loop);
      state[13] += rotr(data, data & 0x1f) + rotr(dataNext, dataNext & 0x1f);
      state[10]  = csum(state[10] + data, dataNext, loop);
      state[11]  = csum(state[11] ^ data, dataNext, loop);
      state[12] += state[8] ^ data;
  }

  u32 buf[4];
  for(auto &b : buf) b = state[0];

  for(loop = 0; loop < 16; loop++) {
      data = state[loop];
      u32 tmp = buf[0] + rotr(data, data & 0x1f);
      buf[0] = tmp;
      buf[1] = data < tmp ? buf[1]+data : csum(buf[1], data, loop);

      tmp = (data & 0x02) >> 1;
      u32 tmp2 = data & 0x01;
      buf[2] = tmp == tmp2 ? buf[2]+data : csum(buf[2], data, loop);
      buf[3] = tmp2 == 1 ? buf[3]^data : csum(buf[3], data, loop);
  }
  
  u64 checksum = (u64)csum(buf[0], buf[1], 16) << 32;
  checksum |= buf[3] ^ buf[2];
  return checksum & 0xffffffffffffull;
}

auto Nintendo64::load(string location) -> LoadResult {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return romNotFound;


  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("id",     document["game/id"].string());
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("cpak",   (bool)document["game/controllerpak"]);
  pak->setAttribute("rpak",   (bool)document["game/rumblepak"]);
  pak->setAttribute("tpak",   (bool)document["game/transferpak"]);
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
  if(auto node = document["game/board/memory(type=RTC,content=Save)"]) {
    Medium::load(node, ".rtc");
  }

  return successful;
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
  if(auto node = document["game/board/memory(type=RTC,content=Save)"]) {
    Medium::save(node, ".rtc");
  }

  return true;
}

auto Nintendo64::analyze(vector<u8>& data) -> string {
  if(data.size() < 0x1000) {
    print("[mia] Loading rom failed. Minimum expected rom size is 4096 (0x1000) bytes. Rom size: ", data.size(), " (0x", hex(data.size()), ") bytes.\n");
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
  case 'W': region = "PAL";  break;  //Scandinavia
  case 'X': region = "PAL";  break;  //Europe
  case 'Y': region = "PAL";  break;  //Europe
  case 'Z': region = "PAL";  break;  //Europe
  }

  string id;
  id.append((char)data[0x3b]);
  id.append((char)data[0x3c]);
  id.append((char)data[0x3d]);

  char region_code = data[0x3e];
  u8 revision = data[0x3f];

  //detect the CIC used by calculating the IPL2 checksum with the various seeds
  //provided by the various CICs, and checking if the checksum matches.
  //this also works for modern IPL3s variants (proprietary or open source),
  //as long as they are used with a CIC we know of.
  bool ntsc = region == "NTSC";
  auto ipl3 = array_view<u8>(&data[0x40], 0xfc0);
  string cic = "";

  if (!cic) switch (ipl2checksum(0x3F, ipl3)) {
    case 0x45cc73ee317aull: cic = "CIC-NUS-6101"; break; //always NTSC (Star Fox 64)
    case 0x44160ec5d9afull: cic = "CIC-NUS-7102"; break; //always PAL (Lylat Wars)
    case 0xa536c0f1d859ull: cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101"; break;
  } 
  if (!cic) switch (ipl2checksum(0x78, ipl3)) {
    case 0x586fd4709867ull: cic = ntsc ? "CIC-NUS-6103" : "CIC-NUS-7103"; break;
  }
  if (!cic) switch (ipl2checksum(0x91, ipl3)) {
    case 0x8618a45bc2d3ull: cic = ntsc ? "CIC-NUS-6105" : "CIC-NUS-7105"; break;
  }
  if (!cic) switch (ipl2checksum(0x85, ipl3)) {
    case 0x2bbad4e6eb74ull: cic = ntsc ? "CIC-NUS-6106" : "CIC-NUS-7106"; break;
  }
  if (!cic) switch (ipl2checksum(0xac, ipl3)) {
    case 0x93e983a8f152ull: cic = "CIC-NUS-5101"; break; //Aleck64 (and conversion hacks)
  }
  if (!cic) switch (ipl2checksum(0xdd, ipl3)) {
    case 0x32b294e2ab90ull: cic = "CIC-NUS-8303"; break; //64DD Retail IPL (Japanese)
    case 0x6ee8d9e84970ull: cic = "CIC-NUS-8401"; break; //64DD Development IPL (Japanese)
    case 0x083c6c77e0b1ull: cic = "CIC-NUS-5167"; break; //64DD Conversion cartridges
    case 0x05ba2ef0a5f1ull: cic = "CIC-NUS-DDUS"; break; //64DD Retail IPL (North American, unreleased)
  }
  if (!cic) cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";  //fallback; most common

  //detect the save type based on the game ID
  u32 eeprom  = 0;      //512_B or 2_KiB
  u32 sram    = 0;      //32_KiB
  u32 flash   = 0;      //128_KiB

  //supported peripherals
  bool cpak = false;                 //Controller Pak
  bool rpak = false;                 //Rumble Pak
  bool tpak = false;                 //Transfer Pak
  bool rtc  = false;                 //RTC
  bool dd   = id.beginsWith("C");    //64DD

  //512B EEPROM
  if(id == "NTW") {eeprom = 512; cpak = true;}                             //64 de Hakken!! Tamagotchi
  if(id == "NHF") {eeprom = 512;}                                          //64 Hanafuda: Tenshi no Yakusoku
  if(id == "NOS") {eeprom = 512; cpak = true; rpak = true;}                //64 Oozumou
  if(id == "NTC") {eeprom = 512; rpak = true;}                             //64 Trump Collection
  if(id == "NER") {eeprom = 512; rpak = true;}                             //Aero Fighters Assault [Sonic Wings Assault (J)]
  if(id == "NAG") {eeprom = 512; cpak = true;}                             //AeroGauge
  if(id == "NAB") {eeprom = 512; cpak = true; rpak = true;}                //Air Boarder 64
  if(id == "NS3") {eeprom = 512; cpak = true;}                             //AI Shougi 3
  if(id == "NTN") {eeprom = 512;}                                          //All Star Tennis '99
  if(id == "NBN") {eeprom = 512; cpak = true;}                             //Bakuretsu Muteki Bangaioh
  if(id == "NBK") {eeprom = 512; rpak = true;}                             //Banjo-Kazooie [Banjo to Kazooie no Daiboken (J)]
  if(id == "NFH") {eeprom = 512; rpak = true;}                             //In-Fisherman Bass Hunter 64 
  if(id == "NMU") {eeprom = 512; cpak = true; rpak = true;}                //Big Mountain 2000
  if(id == "NBC") {eeprom = 512; cpak = true;}                             //Blast Corps
  if(id == "NBH") {eeprom = 512; rpak = true;}                             //Body Harvest
  if(id == "NHA") {eeprom = 512; cpak = true;}                             //Bomberman 64: Arcade Edition (J)
  if(id == "NBM") {eeprom = 512; cpak = true;}                             //Bomberman 64 [Baku Bomberman (J)]
  if(id == "NBV") {eeprom = 512; cpak = true; rpak = true;}                //Bomberman 64: The Second Attack! [Baku Bomberman 2 (J)]
  if(id == "NBD") {eeprom = 512; rpak = true;}                             //Bomberman Hero [Mirian Ojo o Sukue! (J)]
  if(id == "NCT") {eeprom = 512; rpak = true;}                             //Chameleon Twist
  if(id == "NCH") {eeprom = 512; rpak = true;}                             //Chopper Attack
  if(id == "NCG") {eeprom = 512; cpak = true; rpak = true; tpak = true; }  //Choro Q 64 II - Hacha Mecha Grand Prix Race (J)
  if(id == "NP2") {eeprom = 512; cpak = true; rpak = true;}                //Chou Kuukan Night Pro Yakyuu King 2 (J)
  if(id == "NXO") {eeprom = 512; rpak = true;}                             //Cruis'n Exotica
  if(id == "NCU") {eeprom = 512; cpak = true;}                             //Cruis'n USA
  if(id == "NCX") {eeprom = 512;}                                          //Custom Robo
  if(id == "NDY") {eeprom = 512; cpak = true; rpak = true;}                //Diddy Kong Racing
  if(id == "NDQ") {eeprom = 512; cpak = true;}                             //Disney's Donald Duck - Goin' Quackers [Quack Attack (E)]
  if(id == "NDR") {eeprom = 512;}                                          //Doraemon: Nobita to 3tsu no Seireiseki
  if(id == "NN6") {eeprom = 512;}                                          //Dr. Mario 64
  if(id == "NDU") {eeprom = 512; rpak = true;}                             //Duck Dodgers starring Daffy Duck
  if(id == "NJM") {eeprom = 512;}                                          //Earthworm Jim 3D
  if(id == "NFW") {eeprom = 512; rpak = true;}                             //F-1 World Grand Prix
  if(id == "NF2") {eeprom = 512; rpak = true;}                             //F-1 World Grand Prix II
  if(id == "NKA") {eeprom = 512; cpak = true; rpak = true;}                //Fighters Destiny [Fighting Cup (J)]
  if(id == "NFG") {eeprom = 512; cpak = true; rpak = true;}                //Fighter Destiny 2
  if(id == "NGL") {eeprom = 512; cpak = true; rpak = true;}                //Getter Love!!
  if(id == "NGV") {eeprom = 512;}                                          //Glover
  if(id == "NGE") {eeprom = 512; rpak = true;}                             //GoldenEye 007
  if(id == "NHP") {eeprom = 512;}                                          //Heiwa Pachinko World 64
  if(id == "NPG") {eeprom = 512; rpak = true;}                             //Hey You, Pikachu! [Pikachu Genki Dechu (J)]
  if(id == "NIJ") {eeprom = 512; rpak = true;}                             //Indiana Jones and the Infernal Machine
  if(id == "NIC") {eeprom = 512; rpak = true;}                             //Indy Racing 2000
  if(id == "NFY") {eeprom = 512; cpak = true; rpak = true;}                //Kakutou Denshou: F-Cup Maniax
  if(id == "NKI") {eeprom = 512; cpak = true;}                             //Killer Instinct Gold
  if(id == "NLL") {eeprom = 512; rpak = true;}                             //Last Legion UX
  if(id == "NLR") {eeprom = 512; rpak = true;}                             //Lode Runner 3-D
  if(id == "NKT") {eeprom = 512; cpak = true;}                             //Mario Kart 64
  if(id == "CLB") {eeprom = 512; rpak = true;}                             //Mario Party (NTSC)
  if(id == "NLB") {eeprom = 512; rpak = true;}                             //Mario Party (PAL)
  if(id == "NMW") {eeprom = 512; rpak = true;}                             //Mario Party 2
  if(id == "NML") {eeprom = 512; rpak = true; tpak = true;}                //Mickey's Speedway USA [Mickey no Racing Challenge USA (J)]
  if(id == "NTM") {eeprom = 512;}                                          //Mischief Makers [Yuke Yuke!! Trouble Makers (J)]
  if(id == "NMI") {eeprom = 512; rpak = true;}                             //Mission: Impossible
  if(id == "NMG") {eeprom = 512; cpak = true; rpak = true;}                //Monaco Grand Prix [Racing Simulation 2 (G)]
  if(id == "NMO") {eeprom = 512;}                                          //Monopoly
  if(id == "NMS") {eeprom = 512; cpak = true;}                             //Morita Shougi 64
  if(id == "NMR") {eeprom = 512; cpak = true; rpak = true;}                //Multi-Racing Championship
  if(id == "NCR") {eeprom = 512; cpak = true;}                             //Penny Racers [Choro Q 64 (J)]
  if(id == "NEA") {eeprom = 512;}                                          //PGA European Tour
  if(id == "NPW") {eeprom = 512;}                                          //Pilotwings 64
  if(id == "NPY") {eeprom = 512; rpak = true;}                             //Puyo Puyo Sun 64
  if(id == "NPT") {eeprom = 512; rpak = true; tpak = true;}                //Puyo Puyon Party
  if(id == "NRA") {eeprom = 512; cpak = true; rpak = true;}                //Rally '99 (J)
  if(id == "NWQ") {eeprom = 512; cpak = true; rpak = true;}                //Rally Challenge 2000
  if(id == "NSU") {eeprom = 512; rpak = true;}                             //Rocket: Robot on Wheels
  if(id == "NSN") {eeprom = 512; cpak = true; rpak = true;}                //Snow Speeder (J)
  if(id == "NK2") {eeprom = 512; rpak = true;}                             //Snowboard Kids 2 [Chou Snobow Kids (J)]
  if(id == "NSV") {eeprom = 512; rpak = true;}                             //Space Station Silicon Valley
  if(id == "NFX") {eeprom = 512; rpak = true;}                             //Star Fox 64 [Lylat Wars (E)]
  if(id == "NS6") {eeprom = 512; rpak = true;}                             //Star Soldier: Vanishing Earth
  if(id == "NNA") {eeprom = 512; rpak = true;}                             //Star Wars Episode I: Battle for Naboo
  if(id == "NRS") {eeprom = 512; rpak = true;}                             //Star Wars: Rogue Squadron [Shutsugeki! Rogue Chuutai (J)]
  if(id == "NSW") {eeprom = 512;}                                          //Star Wars: Shadows of the Empire [Teikoku no Kage (J)]
  if(id == "NSC") {eeprom = 512;}                                          //Starshot: Space Circus Fever
  if(id == "NSA") {eeprom = 512; rpak = true;}                             //Sonic Wings Assault (J)
  if(id == "NB6") {eeprom = 512; cpak = true; tpak = true;}                //Super B-Daman: Battle Phoenix 64
  if(id == "NSS") {eeprom = 512; rpak = true;}                             //Super Robot Spirits
  if(id == "NTX") {eeprom = 512; rpak = true;}                             //Taz Express
  if(id == "NT6") {eeprom = 512;}                                          //Tetris 64
  if(id == "NTP") {eeprom = 512;}                                          //Tetrisphere
  if(id == "NTJ") {eeprom = 512; rpak = true;}                             //Tom & Jerry in Fists of Fury
  if(id == "NRC") {eeprom = 512; rpak = true;}                             //Top Gear Overdrive
  if(id == "NTR") {eeprom = 512; cpak = true; rpak = true;}                //Top Gear Rally (J + E)
  if(id == "NTB") {eeprom = 512; rpak = true;}                             //Transformers: Beast Wars Metals 64 (J)
  if(id == "NGU") {eeprom = 512; rpak = true;}                             //Tsumi to Batsu: Hoshi no Keishousha (Sin and Punishment)
  if(id == "NIR") {eeprom = 512; rpak = true;}                             //Utchan Nanchan no Hono no Challenger: Denryuu Ira Ira Bou
  if(id == "NVL") {eeprom = 512; rpak = true;}                             //V-Rally Edition '99
  if(id == "NVY") {eeprom = 512; rpak = true;}                             //V-Rally Edition '99 (J)
  if(id == "NWC") {eeprom = 512; rpak = true;}                             //Wild Choppers
  if(id == "NAD") {eeprom = 512;}                                          //Worms Armageddon (U)
  if(id == "NWU") {eeprom = 512;}                                          //Worms Armageddon (E)
  if(id == "NYK") {eeprom = 512; rpak = true;}                             //Yakouchuu II: Satsujin Kouro
  if(id == "NMZ") {eeprom = 512;}                                          //Zool - Majou Tsukai Densetsu (J)

  //2KB EEPROM
  if(id == "NB7") {eeprom = 2_KiB; rpak = true;}                           //Banjo-Tooie [Banjo to Kazooie no Daiboken 2 (J)]
  if(id == "NGT") {eeprom = 2_KiB; cpak = true; rpak = true;}              //City Tour GrandPrix - Zen Nihon GT Senshuken
  if(id == "NFU") {eeprom = 2_KiB; rpak = true;}                           //Conker's Bad Fur Day
  if(id == "NCW") {eeprom = 2_KiB; rpak = true;}                           //Cruis'n World
  if(id == "NCZ") {eeprom = 2_KiB; rpak = true;}                           //Custom Robo V2
  if(id == "ND6") {eeprom = 2_KiB; rpak = true;}                           //Densha de Go! 64
  if(id == "NDO") {eeprom = 2_KiB; rpak = true;}                           //Donkey Kong 64
  if(id == "ND2") {eeprom = 2_KiB; rpak = true;}                           //Doraemon 2: Nobita to Hikari no Shinden
  if(id == "N3D") {eeprom = 2_KiB; rpak = true;}                           //Doraemon 3: Nobita no Machi SOS!
  if(id == "NMX") {eeprom = 2_KiB; cpak = true; rpak = true;}              //Excitebike 64
  if(id == "NGC") {eeprom = 2_KiB; cpak = true; rpak = true;}              //GT 64: Championship Edition
  if(id == "NIM") {eeprom = 2_KiB;}                                        //Ide Yosuke no Mahjong Juku
  if(id == "NNB") {eeprom = 2_KiB; cpak = true; rpak = true;}              //Kobe Bryant in NBA Courtside
  if(id == "NMV") {eeprom = 2_KiB; rpak = true;}                           //Mario Party 3
  if(id == "NM8") {eeprom = 2_KiB; rpak = true; tpak = true;}              //Mario Tennis
  if(id == "NEV") {eeprom = 2_KiB; rpak = true;}                           //Neon Genesis Evangelion
  if(id == "NPP") {eeprom = 2_KiB; cpak = true;}                           //Parlor! Pro 64: Pachinko Jikki Simulation Game
  if(id == "NUB") {eeprom = 2_KiB; cpak = true; tpak = true;}              //PD Ultraman Battle Collection 64
  if(id == "NPD") {eeprom = 2_KiB; cpak = true; rpak = true; tpak = true;} //Perfect Dark
  if(id == "NRZ") {eeprom = 2_KiB; rpak = true;}                           //Ridge Racer 64
  if(id == "NR7") {eeprom = 2_KiB; tpak = true;}                           //Robot Poncots 64: 7tsu no Umi no Caramel
  if(id == "NEP") {eeprom = 2_KiB; rpak = true;}                           //Star Wars Episode I: Racer
  if(id == "NYS") {eeprom = 2_KiB; rpak = true;}                           //Yoshi's Story

  //32KB SRAM
  if(id == "NTE") {sram = 32_KiB; rpak = true;}                            //1080 Snowboarding
  if(id == "NVB") {sram = 32_KiB; rpak = true;}                            //Bass Rush - ECOGEAR PowerWorm Championship (J)
  if(id == "NB5") {sram = 32_KiB; rpak = true;}                            //Biohazard 2 (J)
  if(id == "CFZ") {sram = 32_KiB; rpak = true;}                            //F-Zero X (J)
  if(id == "NFZ") {sram = 32_KiB; rpak = true;}                            //F-Zero X (U + E)
  if(id == "NSI") {sram = 32_KiB; cpak = true;}                            //Fushigi no Dungeon: Fuurai no Shiren 2
  if(id == "NG6") {sram = 32_KiB; rpak = true;}                            //Ganmare Goemon: Dero Dero Douchuu Obake Tenkomori
  if(id == "NGP") {sram = 32_KiB; cpak = true;}                            //Goemon: Mononoke Sugoroku
  if(id == "NYW") {sram = 32_KiB;}                                         //Harvest Moon 64
  if(id == "NHY") {sram = 32_KiB; cpak = true; rpak = true;}               //Hybrid Heaven (J)
  if(id == "NIB") {sram = 32_KiB; rpak = true;}                            //Itoi Shigesato no Bass Tsuri No. 1 Kettei Ban!
  if(id == "NPS") {sram = 32_KiB; cpak = true; rpak = true;}               //Jikkyou J.League 1999: Perfect Striker 2
  if(id == "NPA") {sram = 32_KiB; cpak = true; tpak = true;}               //Jikkyou Powerful Pro Yakyuu 2000
  if(id == "NP4") {sram = 32_KiB; cpak = true;}                            //Jikkyou Powerful Pro Yakyuu 4
  if(id == "NJ5") {sram = 32_KiB; cpak = true;}                            //Jikkyou Powerful Pro Yakyuu 5
  if(id == "NP6") {sram = 32_KiB; cpak = true; tpak = true;}               //Jikkyou Powerful Pro Yakyuu 6
  if(id == "NPE") {sram = 32_KiB; cpak = true;}                            //Jikkyou Powerful Pro Yakyuu Basic Ban 2001
  if(id == "NJG") {sram = 32_KiB; rpak = true;}                            //Jinsei Game 64
  if(id == "CZL") {sram = 32_KiB; rpak = true;}                            //Legend of Zelda: Ocarina of Time [Zelda no Densetsu - Toki no Ocarina (J)]
  if(id == "NZL") {sram = 32_KiB; rpak = true;}                            //Legend of Zelda: Ocarina of Time (E)
  if(id == "NKG") {sram = 32_KiB; cpak = true; rpak = true;}               //Major League Baseball featuring Ken Griffey Jr.
  if(id == "NMF") {sram = 32_KiB; rpak = true; tpak = true;}               //Mario Golf 64
  if(id == "NRI") {sram = 32_KiB; cpak = true;}                            //New Tetris, The
  if(id == "NUT") {sram = 32_KiB; cpak = true; rpak = true; tpak = true;}  //Nushi Zuri 64
  if(id == "NUM") {sram = 32_KiB; rpak = true; tpak = true;}               //Nushi Zuri 64: Shiokaze ni Notte
  if(id == "NOB") {sram = 32_KiB;}                                         //Ogre Battle 64: Person of Lordly Caliber
  if(id == "CPS") {sram = 32_KiB; tpak = true;}                            //Pocket Monsters Stadium (J)
  if(id == "NPM") {sram = 32_KiB; cpak = true;}                            //Premier Manager 64
  if(id == "NRE") {sram = 32_KiB; rpak = true;}                            //Resident Evil 2
  if(id == "NAL") {sram = 32_KiB; rpak = true;}                            //Super Smash Bros. [Nintendo All-Star! Dairantou Smash Brothers (J)]
  if(id == "NT3") {sram = 32_KiB; cpak = true;}                            //Shin Nihon Pro Wrestling - Toukon Road 2 - The Next Generation (J)
  if(id == "NS4") {sram = 32_KiB; cpak = true; tpak = true;}               //Super Robot Taisen 64
  if(id == "NA2") {sram = 32_KiB; cpak = true; rpak = true;}               //Virtual Pro Wrestling 2
  if(id == "NVP") {sram = 32_KiB; cpak = true; rpak = true;}               //Virtual Pro Wrestling 64
  if(id == "NWL") {sram = 32_KiB; rpak = true;}                            //Waialae Country Club: True Golf Classics
  if(id == "NW2") {sram = 32_KiB; rpak = true;}                            //WCW-nWo Revenge
  if(id == "NWX") {sram = 32_KiB; cpak = true; rpak = true;}               //WWF WrestleMania 2000

  //96KB SRAM
  if(id == "CDZ") {sram = 96_KiB; rpak = true;}                            //Dezaemon 3D

  //128KB Flash
  if(id == "NCC") {flash = 128_KiB; rpak = true;}                          //Command & Conquer
  if(id == "NDA") {flash = 128_KiB; cpak = true;}                          //Derby Stallion 64
  if(id == "NAF") {flash = 128_KiB; cpak = true; rtc = true;}              //Doubutsu no Mori
  if(id == "NJF") {flash = 128_KiB; rpak = true;}                          //Jet Force Gemini [Star Twins (J)]
  if(id == "NKJ") {flash = 128_KiB; rpak = true;}                          //Ken Griffey Jr.'s Slugfest
  if(id == "NZS") {flash = 128_KiB; rpak = true;}                          //Legend of Zelda: Majora's Mask [Zelda no Densetsu - Mujura no Kamen (J)]
  if(id == "NM6") {flash = 128_KiB; rpak = true;}                          //Mega Man 64
  if(id == "NCK") {flash = 128_KiB; rpak = true;}                          //NBA Courtside 2 featuring Kobe Bryant
  if(id == "NMQ") {flash = 128_KiB; rpak = true;}                          //Paper Mario
  if(id == "NPN") {flash = 128_KiB;}                                       //Pokemon Puzzle League
  if(id == "NPF") {flash = 128_KiB;}                                       //Pokemon Snap [Pocket Monsters Snap (J)]
  if(id == "NPO") {flash = 128_KiB; tpak = true;}                          //Pokemon Stadium
  if(id == "CP2") {flash = 128_KiB; tpak = true;}                          //Pocket Monsters Stadium 2 (J)
  if(id == "NP3") {flash = 128_KiB; tpak = true;}                          //Pokemon Stadium 2 [Pocket Monsters Stadium - Kin Gin (J)]
  if(id == "NRH") {flash = 128_KiB; rpak = true;}                          //Rockman Dash - Hagane no Boukenshin (J)
  if(id == "NSQ") {flash = 128_KiB; rpak = true;}                          //StarCraft 64
  if(id == "NT9") {flash = 128_KiB;}                                       //Tigger's Honey Hunt
  if(id == "NW4") {flash = 128_KiB; cpak = true; rpak = true;}             //WWF No Mercy
  if(id == "NDP") {flash = 128_KiB;}                                       //Dinosaur Planet (Unlicensed)

  //Controller Pak
  if(id == "N4W") {cpak = true; rpak = true;}                              //40 Winks (Aftermarket)
  if(id == "NO7") {cpak = true; rpak = true;}                              //The World Is Not Enough
  if(id == "NAY") {cpak = true;}                                           //Aidyn Chronicles - The First Mage
  if(id == "NBS") {cpak = true; rpak = true;}                              //All-Star Baseball '99
  if(id == "NBE") {cpak = true; rpak = true;}                              //All-Star Baseball 2000
  if(id == "NAS") {cpak = true; rpak = true;}                              //All-Star Baseball 2001
  if(id == "NAR") {cpak = true; rpak = true;}                              //Armorines - Project S.W.A.R.M.
  if(id == "NAC") {cpak = true; rpak = true;}                              //Army Men - Air Combat
  if(id == "NAM") {cpak = true; rpak = true;}                              //Army Men - Sarge's Heroes
  if(id == "N32") {cpak = true; rpak = true;}                              //Army Men - Sarge's Heroes 2
  if(id == "NAH") {cpak = true; rpak = true;}                              //Asteroids Hyper 64
  if(id == "NLC") {cpak = true; rpak = true;}                              //Automobili Lamborghini [Super Speed Race 64 (J)]
  if(id == "NBJ") {cpak = true;}                                           //Bakushou Jinsei 64 - Mezase! Resort Ou
  if(id == "NB4") {cpak = true; rpak = true;}                              //Bass Masters 2000
  if(id == "NBX") {cpak = true; rpak = true;}                              //Battletanx
  if(id == "NBQ") {cpak = true; rpak = true;}                              //Battletanx - Global Assault
  if(id == "NZO") {cpak = true; rpak = true;}                              //Battlezone - Rise of the Black Dogs
  if(id == "NNS") {cpak = true; rpak = true;}                              //Beetle Adventure Racing
  if(id == "NB8") {cpak = true; rpak = true;}                              //Beetle Adventure Racing (J)
  if(id == "NBF") {cpak = true; rpak = true;}                              //Bio F.R.E.A.K.S.
  if(id == "NBP") {cpak = true; rpak = true;}                              //Blues Brothers 2000
  if(id == "NYW") {cpak = true;}                                           //Bokujou Monogatari 2
  if(id == "NBO") {cpak = true;}                                           //Bottom of the 9th
  if(id == "NOW") {cpak = true;}                                           //Brunswick Circuit Pro Bowling
  if(id == "NBL") {cpak = true; rpak = true;}                              //Buck Bumble
  if(id == "NBY") {cpak = true; rpak = true;}                              //Bug's Life, A
  if(id == "NB3") {cpak = true; rpak = true;}                              //Bust-A-Move '99 [Bust-A-Move 3 DX (E)]
  if(id == "NBU") {cpak = true;}                                           //Bust-A-Move 2 - Arcade Edition
  if(id == "NCL") {cpak = true; rpak = true;}                              //California Speed
  if(id == "NCD") {cpak = true; rpak = true;}                              //Carmageddon 64
  if(id == "NTS") {cpak = true;}                                           //Centre Court Tennis [Let's Smash (J)]
  if(id == "N2V") {cpak = true; rpak = true;}                              //Chameleon Twist 2 (U + E)
  if(id == "NV2") {cpak = true; rpak = true;}                              //Chameleon Twist 2 (J)
  if(id == "NPK") {cpak = true;}                                           //Chou Kuukan Night Pro Yakyuu King (J)
  if(id == "NT4") {cpak = true; rpak = true;}                              //CyberTiger
  if(id == "NDW") {cpak = true; rpak = true;}                              //Daikatana, John Romero's
  if(id == "NGA") {cpak = true; rpak = true;}                              //Deadly Arts [G.A.S.P!! Fighter's NEXTream (E + J)]
  if(id == "NDE") {cpak = true; rpak = true;}                              //Destruction Derby 64
  if(id == "NDQ") {cpak = true;}                                           //Disney's Donald Duck - Goin' Quackers [Quack Attack (E)]
  if(id == "NTA") {cpak = true; rpak = true;}                              //Disney's Tarzan
  if(id == "NDM") {cpak = true;}                                           //Doom 64
  if(id == "NDH") {cpak = true;}                                           //Duel Heroes
  if(id == "NDN") {cpak = true; rpak = true;}                              //Duke Nukem 64
  if(id == "NDZ") {cpak = true; rpak = true;}                              //Duke Nukem - Zero Hour
  if(id == "NWI") {cpak = true; rpak = true;}                              //ECW Hardcore Revolution
  if(id == "NST") {cpak = true;}                                           //Eikou no Saint Andrews
  if(id == "NET") {cpak = true;}                                           //Quest 64 [Eltale Monsters (J) Holy Magic Century (E)]
  if(id == "NEG") {cpak = true; rpak = true;}                              //Extreme-G
  if(id == "NG2") {cpak = true; rpak = true;}                              //Extreme-G XG2
  if(id == "NHG") {cpak = true;}                                           //F-1 Pole Position 64
  if(id == "NFR") {cpak = true; rpak = true;}                              //F-1 Racing Championship
  if(id == "N8I") {cpak = true;}                                           //FIFA - Road to World Cup 98 [World Cup e no Michi (J)]
  if(id == "N9F") {cpak = true;}                                           //FIFA 99
  if(id == "N7I") {cpak = true;}                                           //FIFA Soccer 64 [FIFA 64 (E)]
  if(id == "NFS") {cpak = true;}                                           //Famista 64
  if(id == "NFF") {cpak = true; rpak = true;}                              //Fighting Force 64
  if(id == "NFD") {cpak = true; rpak = true;}                              //Flying Dragon
  if(id == "NFO") {cpak = true; rpak = true;}                              //Forsaken 64
  if(id == "NF9") {cpak = true;}                                           //Fox Sports College Hoops '99
  if(id == "NG5") {cpak = true; rpak = true;}                              //Ganbare Goemon - Neo Momoyama Bakufu no Odori [Mystical Ninja Starring Goemon]
  if(id == "NGX") {cpak = true; rpak = true;}                              //Gauntlet Legends
  if(id == "NGD") {cpak = true; rpak = true;}                              //Gauntlet Legends (J)
  if(id == "NX3") {cpak = true; rpak = true;}                              //Gex 3 - Deep Cover Gecko
  if(id == "NX2") {cpak = true;}                                           //Gex 64 - Enter the Gecko
  if(id == "NGM") {cpak = true; rpak = true;}                              //Goemon's Great Adventure [Mystical Ninja 2 Starring Goemon]
  if(id == "NGN") {cpak = true;}                                           //Golden Nugget 64
  if(id == "NHS") {cpak = true;}                                           //Hamster Monogatari 64
  if(id == "NM9") {cpak = true;}                                           //Harukanaru Augusta Masters 98
  if(id == "NHC") {cpak = true; rpak = true;}                              //Hercules - The Legendary Journeys
  if(id == "NHX") {cpak = true;}                                           //Hexen
  if(id == "NHK") {cpak = true; rpak = true;}                              //Hiryuu no Ken Twin
  if(id == "NHW") {cpak = true; rpak = true;}                              //Hot Wheels Turbo Racing
  if(id == "NHG") {cpak = true;}                                           //Human Grand Prix - New Generation 
  if(id == "NHV") {cpak = true; rpak = true;}                              //Hybrid Heaven (U + E)
  if(id == "NHT") {cpak = true; rpak = true;}                              //Hydro Thunder
  if(id == "NWB") {cpak = true; rpak = true;}                              //Iggy's Reckin' Balls [Iggy-kun no Bura Bura Poyon (J)]
  if(id == "NWS") {cpak = true;}                                           //International Superstar Soccer '98 [Jikkyo World Soccer - World Cup France '98 (J)]
  if(id == "NIS") {cpak = true; rpak = true;}                              //International Superstar Soccer 2000 
  if(id == "NJP") {cpak = true;}                                           //International Superstar Soccer 64 [Jikkyo J-League Perfect Striker (J)]
  if(id == "NDS") {cpak = true;}                                           //J.League Dynamite Soccer 64
  if(id == "NJE") {cpak = true;}                                           //J.League Eleven Beat 1997
  if(id == "NJL") {cpak = true;}                                           //J.League Live 64
  if(id == "NMA") {cpak = true;}                                           //Jangou Simulation Mahjong Do 64
  if(id == "NCO") {cpak = true; rpak = true;}                              //Jeremy McGrath Supercross 2000
  if(id == "NGS") {cpak = true;}                                           //Jikkyou G1 Stable
  if(id == "NJ3") {cpak = true;}                                           //Jikkyou World Soccer 3
  if(id == "N64") {cpak = true; rpak = true;}                              //Kira to Kaiketsu! 64 Tanteidan
  if(id == "NKK") {cpak = true; rpak = true;}                              //Knockout Kings 2000
  if(id == "NLG") {cpak = true; rpak = true;}                              //LEGO Racers
  if(id == "N8M") {cpak = true; rpak = true;}                              //Madden Football 64
  if(id == "NMD") {cpak = true; rpak = true;}                              //Madden Football 2000
  if(id == "NFL") {cpak = true; rpak = true;}                              //Madden Football 2001
  if(id == "N2M") {cpak = true; rpak = true;}                              //Madden Football 2002
  if(id == "N9M") {cpak = true; rpak = true;}                              //Madden Football '99
  if(id == "NMJ") {cpak = true;}                                           //Mahjong 64
  if(id == "NMM") {cpak = true;}                                           //Mahjong Master
  if(id == "NHM") {cpak = true; rpak = true;}                              //Mia Hamm Soccer 64
  if(id == "NWK") {cpak = true; rpak = true;}                              //Michael Owens WLS 2000 [World League Soccer 2000 (E) / Telefoot Soccer 2000 (F)]
  if(id == "NV3") {cpak = true; rpak = true;}                              //Micro Machines 64 Turbo
  if(id == "NAI") {cpak = true;}                                           //Midway's Greatest Arcade Hits Volume 1
  if(id == "NMB") {cpak = true; rpak = true;}                              //Mike Piazza's Strike Zone
  if(id == "NBR") {cpak = true; rpak = true;}                              //Milo's Astro Lanes
  if(id == "NM4") {cpak = true; rpak = true;}                              //Mortal Kombat 4
  if(id == "NMY") {cpak = true; rpak = true;}                              //Mortal Kombat Mythologies - Sub-Zero
  if(id == "NP9") {cpak = true; rpak = true;}                              //Ms. Pac-Man - Maze Madness
  if(id == "NH5") {cpak = true;}                                           //Nagano Winter Olympics '98 [Hyper Olympics in Nagano (J)]
  if(id == "NNM") {cpak = true;}                                           //Namco Museum 64
  if(id == "N9C") {cpak = true; rpak = true;}                              //Nascar '99
  if(id == "NN2") {cpak = true; rpak = true;}                              //Nascar 2000
  if(id == "NXG") {cpak = true;}                                           //NBA Hangtime
  if(id == "NBA") {cpak = true; rpak = true;}                              //NBA In the Zone '98 [NBA Pro '98 (E)]
  if(id == "NB2") {cpak = true; rpak = true;}                              //NBA In the Zone '99 [NBA Pro '99 (E)]
  if(id == "NWZ") {cpak = true; rpak = true;}                              //NBA In the Zone 2000
  if(id == "NB9") {cpak = true;}                                           //NBA Jam '99
  if(id == "NJA") {cpak = true; rpak = true;}                              //NBA Jam 2000
  if(id == "N9B") {cpak = true; rpak = true;}                              //NBA Live '99
  if(id == "NNL") {cpak = true; rpak = true;}                              //NBA Live 2000
  if(id == "NSO") {cpak = true;}                                           //NBA Showtime - NBA on NBC
  if(id == "NBZ") {cpak = true; rpak = true;}                              //NFL Blitz
  if(id == "NSZ") {cpak = true; rpak = true;}                              //NFL Blitz - Special Edition
  if(id == "NBI") {cpak = true; rpak = true;}                              //NFL Blitz 2000
  if(id == "NFB") {cpak = true; rpak = true;}                              //NFL Blitz 2001
  if(id == "NQ8") {cpak = true; rpak = true;}                              //NFL Quarterback Club '98
  if(id == "NQ9") {cpak = true; rpak = true;}                              //NFL Quarterback Club '99
  if(id == "NQB") {cpak = true; rpak = true;}                              //NFL Quarterback Club 2000
  if(id == "NQC") {cpak = true; rpak = true;}                              //NFL Quarterback Club 2001
  if(id == "N9H") {cpak = true; rpak = true;}                              //NHL '99
  if(id == "NHO") {cpak = true; rpak = true;}                              //NHL Blades of Steel '99 [NHL Pro '99 (E)]
  if(id == "NHL") {cpak = true; rpak = true;}                              //NHL Breakaway '98
  if(id == "NH9") {cpak = true; rpak = true;}                              //NHL Breakaway '99
  if(id == "NNC") {cpak = true; rpak = true;}                              //Nightmare Creatures
  if(id == "NCE") {cpak = true; rpak = true;}                              //Nuclear Strike 64
  if(id == "NTD") {cpak = true; rpak = true;}                              //O.D.T. (Unreleased)
  if(id == "NOF") {cpak = true; rpak = true;}                              //Offroad Challenge
  if(id == "NHN") {cpak = true;}                                           //Olympic Hockey Nagano '98
  if(id == "NOM") {cpak = true;}                                           //Onegai Monsters
  if(id == "NPC") {cpak = true;}                                           //Pachinko 365 Nichi (J)
  if(id == "NYP") {cpak = true; rpak = true;}                              //Paperboy
  if(id == "NPX") {cpak = true; rpak = true;}                              //Polaris SnoCross
  if(id == "NPL") {cpak = true;}                                           //Power League 64 (J)
  if(id == "NPU") {cpak = true;}                                           //Power Rangers - Lightspeed Rescue
  if(id == "NKM") {cpak = true;}                                           //Pro Mahjong Kiwame 64 (J)
  if(id == "NNR") {cpak = true;}                                           //Pro Mahjong Tsuwamono 64 - Jansou Battle ni Chousen (J)
  if(id == "NPB") {cpak = true; rpak = true;}                              //Puzzle Bobble 64 (J)
  if(id == "NQK") {cpak = true; rpak = true;}                              //Quake 64
  if(id == "NQ2") {cpak = true; rpak = true;}                              //Quake 2
  if(id == "NKR") {cpak = true;}                                           //Rakuga Kids (E)
  if(id == "NRP") {cpak = true; rpak = true;}                              //Rampage - World Tour
  if(id == "NRP") {cpak = true; rpak = true;}                              //Rampage 2 - Universal Tour
  if(id == "NRT") {cpak = true;}                                           //Rat Attack
  if(id == "NY2") {cpak = true;}                                           //Rayman 2 - The Great Escape
  if(id == "NFQ") {cpak = true; rpak = true;}                              //Razor Freestyle Scooter
  if(id == "NRV") {cpak = true; rpak = true;}                              //Re-Volt
  if(id == "NRD") {cpak = true; rpak = true;}                              //Ready 2 Rumble Boxing
  if(id == "N22") {cpak = true; rpak = true;}                              //Ready 2 Rumble Boxing - Round 2
  if(id == "NRO") {cpak = true; rpak = true;}                              //Road Rash 64
  if(id == "NRR") {cpak = true; rpak = true;}                              //Roadster's Trophy
  if(id == "NRT") {cpak = true;}                                           //Robotron 64
  if(id == "NRK") {cpak = true;}                                           //Rugrats in Paris - The Movie
  if(id == "NR2") {cpak = true; rpak = true;}                              //Rush 2 - Extreme Racing USA
  if(id == "NCS") {cpak = true; rpak = true;}                              //S.C.A.R.S.
  if(id == "NDC") {cpak = true; rpak = true;}                              //SD Hiryuu no Ken Densetsu (J)
  if(id == "NSH") {cpak = true;}                                           //Saikyou Habu Shougi (J)
  if(id == "NSF") {cpak = true; rpak = true;}                              //San Francisco Rush - Extreme Racing
  if(id == "NRU") {cpak = true; rpak = true;}                              //San Francisco Rush 2049
  if(id == "NSY") {cpak = true;}                                           //Scooby-Doo! - Classic Creep Capers
  if(id == "NSD") {cpak = true; rpak = true;}                              //Shadow Man
  if(id == "NSG") {cpak = true;}                                           //Shadowgate 64 - Trials Of The Four Towers
  if(id == "NTO") {cpak = true;}                                           //Shin Nihon Pro Wrestling - Toukon Road - Brave Spirits (J)
  if(id == "NS2") {cpak = true;}                                           //Simcity 2000
  if(id == "NSK") {cpak = true; rpak = true;}                              //Snowboard Kids [Snobow Kids (J)]
  if(id == "NDT") {cpak = true; rpak = true;}                              //South Park
  if(id == "NPR") {cpak = true; rpak = true;}                              //South Park Rally
  if(id == "NIV") {cpak = true; rpak = true;}                              //Space Invaders
  if(id == "NSL") {cpak = true; rpak = true;}                              //Spider-Man
  if(id == "NR3") {cpak = true; rpak = true;}                              //Stunt Racer 64
  if(id == "NBW") {cpak = true; rpak = true;}                              //Super Bowling
  if(id == "NSX") {cpak = true; rpak = true;}                              //Supercross 2000
  if(id == "NSP") {cpak = true; rpak = true;}                              //Superman
  if(id == "NPZ") {cpak = true; rpak = true;}                              //Susume! Taisen Puzzle Dama Toukon! Marumata Chou (J)
  if(id == "NL2") {cpak = true; rpak = true;}                              //Top Gear Rally 2
  if(id == "NR6") {cpak = true; rpak = true;}                              //Tom Clancy's Rainbow Six
  if(id == "NTT") {cpak = true;}                                           //Tonic Trouble
  if(id == "NTF") {cpak = true; rpak = true;}                              //Tony Hawk's Pro Skater
  if(id == "NTQ") {cpak = true; rpak = true;}                              //Tony Hawk's Pro Skater 2
  if(id == "NT3") {cpak = true; rpak = true;}                              //Tony Hawk's Pro Skater 3
  if(id == "NGB") {cpak = true; rpak = true;}                              //Top Gear Hyper Bike
  if(id == "NGR") {cpak = true; rpak = true;}                              //Top Gear Rally (U)
  if(id == "NTH") {cpak = true; rpak = true;}                              //Toy Story 2 - Buzz Lightyear to the Rescue!
  if(id == "N3P") {cpak = true; rpak = true;}                              //Triple Play 2000
  if(id == "NTU") {cpak = true;}                                           //Turok: Dinosaur Hunter [Turok: Jikuu Senshi (J)]
  if(id == "NRW") {cpak = true; rpak = true;}                              //Turok: Rage Wars
  if(id == "NT2") {cpak = true; rpak = true;}                              //Turok 2 - Seeds of Evil [Violence Killer - Turok New Generation (J)]
  if(id == "NTK") {cpak = true; rpak = true;}                              //Turok 3 - Shadow of Oblivion
  if(id == "NSB") {cpak = true; rpak = true;}                              //Twisted Edge - Extreme Snowboarding [King Hill 64 - Extreme Snowboarding (J)]
  if(id == "NV8") {cpak = true; rpak = true;}                              //Vigilante 8
  if(id == "NVG") {cpak = true; rpak = true;}                              //Vigilante 8 - Second Offense
  if(id == "NVC") {cpak = true;}                                           //Virtual Chess 64
  if(id == "NVR") {cpak = true;}                                           //Virtual Pool 64
  if(id == "NWV") {cpak = true; rpak = true;}                              //WCW: Backstage Assault
  if(id == "NWM") {cpak = true; rpak = true;}                              //WCW: Mayhem
  if(id == "NW3") {cpak = true; rpak = true;}                              //WCW: Nitro
  if(id == "NWN") {cpak = true; rpak = true;}                              //WCW vs. nWo - World Tour
  if(id == "NWW") {cpak = true; rpak = true;}                              //WWF: War Zone
  if(id == "NTI") {cpak = true; rpak = true;}                              //WWF: Attitude
  if(id == "NWG") {cpak = true;}                                           //Wayne Gretzky's 3D Hockey
  if(id == "NW8") {cpak = true;}                                           //Wayne Gretzky's 3D Hockey '98
  if(id == "NWD") {cpak = true; rpak = true;}                              //Winback - Covert Operations
  if(id == "NWP") {cpak = true; rpak = true;}                              //Wipeout 64
  if(id == "NJ2") {cpak = true;}                                           //Wonder Project J2 - Koruro no Mori no Jozet (J)
  if(id == "N8W") {cpak = true;}                                           //World Cup '98
  if(id == "NWO") {cpak = true; rpak = true;}                              //World Driver Championship
  if(id == "NXF") {cpak = true; rpak = true;}                              //Xena Warrior Princess - The Talisman of Fate
  if(id == "NXC") {cpak = true; rpak = true;}                              //Xeno Crisis (Aftermarket)

  //Rumble Pak
  if(id == "NJQ") {rpak = true;}                                           //Batman Beyond - Return of the Joker [Batman of the Future - Return of the Joker (E)]
  if(id == "NCB") {rpak = true;}                                           //Charlie Blast's Territory
  if(id == "NDF") {rpak = true;}                                           //Dance Dance Revolution - Disney Dancing Museum
  if(id == "NKE") {rpak = true;}                                           //Knife Edge - Nose Gunner
  if(id == "NMT") {rpak = true;}                                           //Magical Tetris Challenge
  if(id == "NM3") {rpak = true;}                                           //Monster Truck Madness 64
  if(id == "NRG") {rpak = true;}                                           //Rugrats - Scavenger Hunt [Treasure Hunt (E)]
  if(id == "NOH") {rpak = true; tpak = true;}                              //Transformers Beast Wars - Transmetals
  if(id == "NWF") {rpak = true;}                                           //Wheel of Fortune

  //Special case for save type in International Track & Field
  if(id == "N3H") { 
    if(region_code == 'J') {sram = 32_KiB;}                                //Ganbare! Nippon! Olympics 2000
    else {cpak = true;}                                                    //International Track & Field 2000|Summer Games
    rpak = true;
  } 

  //Special cases for Japanese versions of Castlevania
  if(id == "ND3") {
    if(region_code == 'J') {eeprom = 2_KiB; rpak = true;}                  //Akumajou Dracula Mokushiroku (J)
    else {cpak = true;}                                                    //Castlevania
  }
  if(id == "ND4") {
    if(region_code == 'J') {eeprom = 2_KiB; rpak = true;}                  //Akumajou Dracula Mokushiroku Gaiden: Legend of Cornell (J)
    else {cpak = true;}                                                    //Castlevania - Legacy of Darkness
  }

  //Special case for Super Mario 64 Shindou Edition   
  if(id == "NSM") {
    if(region_code == 'J' && revision == 3) {rpak = true;}
    eeprom = 512;                                                        
  }                                                    

  //Special case for Wave Race 64 Shindou Edition
  if(id == "NWR") {
    if(region_code == 'J' && revision == 2) {rpak = true;}
    eeprom = 512; 
    cpak = true;
  }

  //Special case for save type in Kirby 64: The Crystal Shards [Hoshi no Kirby 64 (J)]
  if(id == "NK4") {                       
    if(region_code == 'J' && revision < 2) {sram = 32_KiB;}
    else {eeprom = 2_KiB;}
    rpak = true;
  }                                              

  //Special case for save type in Dark Rift [Space Dynamites (J)]
  if(id == "NDK" && region_code == 'J') {eeprom = 512;}
  
  //Special case for save type in Wetrix
  if(id == "NWT") { 
    if(region_code == 'J') {eeprom = 512;}
    else {cpak = true;}
  }

  //Homebrew (libdragon / Everdrive special header flag)
  if(id(1) == 'E' && id(2) == 'D') {
    n8 config = data[0x3f];
    if(config.bit(4,7) == 1) {eeprom = 512;}
    if(config.bit(4,7) == 2) {eeprom = 2_KiB;}
    if(config.bit(4,7) == 3) {sram = 32_KiB;}
    //if(config.bit(4,7) == 4) {sram = 96_KiB;}   //banked SRAM, not supported yet
    if(config.bit(4,7) == 5) {flash = 128_KiB;}
    if(config.bit(4,7) == 6) {sram = 128_KiB;}
    if(config.bit(0) == 1)   {rtc = true;}
    
    //Advanced Homebrew ROM Header
    //Controllers
    n8 controller_1 = data[0x34];
    n8 controller_2 = data[0x35];
    n8 controller_3 = data[0x36];
    n8 controller_4 = data[0x37];
    
    auto read_controller_config = [&](n8 controller_config) 
    { 
        switch(controller_config) {
            case 0x00: //default
                rpak = true;
                cpak = true; //TODO rumble does not work for homebrew because of that
                break;
            case 0x01: //N64 controller with Rumble Pak
                rpak = true;
                break;
            case 0x02: //N64 controller with Controller Pak
                cpak = true;
                break;
            case 0x03: //N64 controller with Transfer Pak
                tpak = true;
                break;
            case 0x80: //N64 mouse
                //not supported yet
                break;
            case 0x81: //VRU
                //not supported yet
                break;
            case 0x82: //Gamecube controller
                //not supported yet
                break;
            case 0x83: //Randnet keyboard
                //not supported yet
                break;
            case 0x84: //Gamecube keyboard
                //not supported yet
                break;
            case 0xFF: //Nothing attached to this port
                //not supported yet
                break;
        }
    };
    
    read_controller_config(controller_1);
    //TODO ares does not differentiate for different controller setups yet in Nintendo64::load()
    //read_controller_config(controller_2);
    //read_controller_config(controller_3);
    //read_controller_config(controller_4);
  }

  string s;
  s += "game\n";
  s +={"  name:     ", Medium::name(location), "\n"};
  s +={"  title:    ", Medium::name(location), "\n"};
  s +={"  sha256:   ", sha256, "\n"};
  s +={"  region:   ", region, "\n"};
  s +={"  id:       ", id, region_code, "\n"};
  if(cpak)
  s += "  controllerpak\n";
  if(rpak)
  s += "  rumblepak\n";
  if(tpak)
  s += "  transferpak\n";
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
  if(rtc) {
  s += "    memory\n";
  s += "      type: RTC\n";
  s +={"      size: 0x", hex(32), "\n"};
  s += "      content: Save\n";
  }
  return s;
}
