struct MasterSystem : Cartridge {
  auto name() -> string override { return "Master System"; }
  auto extensions() -> vector<string> override { return {"ms", "sms"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto MasterSystem::load(string location) -> LoadResult {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  } else {
    return romNotFound;
  }

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("board",  document["game/board" ].string());
  pak->setAttribute("title",  document["game/title" ].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("paddle", (bool)document["game/paddle"]);
  pak->setAttribute("sportspad", (bool)document["game/sportspad"]);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }

  return successful;
}

auto MasterSystem::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }

  return true;
}

auto MasterSystem::analyze(vector<u8>& rom) -> string {
  string hash   = Hash::SHA256(rom).digest();
  string board  = "Sega";
  string region = "NTSC-J, NTSC-U, PAL";  //database required to detect region
  u32 ram       = 32_KiB;                 //database required to detect RAM size
  bool paddle = false;
  bool sportspad = false;

  //Region
  //======

  //Games not containing "TMR SEGA" are only compatible with Japanese systems
  if((rom.size() >= 0x8000 && slice((const char*)&rom[0x7ff0], 0, 8) != "TMR SEGA") &&
     (rom.size() >= 0x4000 && slice((const char*)&rom[0x3ff0], 0, 8) != "TMR SEGA") &&
     (rom.size() >= 0x2000 && slice((const char*)&rom[0x1ff0], 0, 8) != "TMR SEGA")) {
    region = "NTSC-J";
  }

  //Addams Family, The (Europe)
  if(hash == "f6967779b78b91a4a5745f142aa5f49463f1d64ec15d6bea2e617323da60b90c") {
    region = "PAL";
  }

  //Back to the Future III (Europe)
  if(hash == "c39167c5dc187e7d4da8ead30b77f10d5b14596ebddf7aa081adf7285e1e8d8d") {
    region = "PAL";
  }

  //BMX Trial - Alex Kidd (Japan)
  if(hash == "0fdd18f1212072bfbc0cfeaf030436d746733029ef26a8a6470c7be101cfedfb") {
    paddle = true;
    region = "NTSC-J";
  }

  //Desert Strike (Europe)
  if(hash == "5465d58ebe359ea630bdb0f0454ef0fc3108d04eb941bfb32434429374926ad2") {
    region = "PAL";
  }

  //Galactic Protector (Japan)
  if(hash == "2d16948696509309493d78c4cd7e9e8db407db593464e5a804155a1e12cf4067") {
    paddle = true;
    region = "NTSC-J";
  }

  //Great Ice Hockey (Japan, USA) (En)
  if(hash == "4b8fe9cb1688c730807fbb68d2f2c135a3a64a6b668af4bca8fed2c3ed050fe1") {
    sportspad = true;
  }

  //Home Alone (Europe)
  if(hash == "2bad719ba9913e767be6cbf5ab60113dfec9db08299a3341e82b96f42eda9ead") {
    region = "PAL";
  }

  //Laser Ghost (Europe)
  if(hash == "a65671102f99f397713cdb952463c7641b5f1d46ce1dd93a3ce681a8ce209894") {
    region = "PAL";
  }

  //Megumi Rescue (Japan)
  if(hash == "d06ec2a0bb145f48695a07d4a6fe374e50ec3dff1a9287c5972b206b74e37c07") {
    paddle = true;
    region = "NTSC-J";
  }

  //NewZealand Story, The (Europe)
  if(hash == "d1e9c377800133ae54cc262f9357481dbac52b20c3b771c546f897e93d4457e3") {
    region = "PAL";
  }

  //PGA Tour Golf (Europe)
  if(hash == "a3856f0d15511f7a6d48fa6c47da2c81adf372e6a04f83298a10cd45b6711530") {
    region = "PAL";
  }

  //Sensible Soccer (Europe)
  if(hash == "85228395287d6bfe9f642e620ccf80de60aabc8e4e6d5cd92001d008895d1e92") {
    region = "PAL";
  }

  //Sports Pad Football (USA)
  if(hash == "70719f8e88cdc58180d5042b0b0cbc2298144acdf062e7e3f14aaecaeb8b50e4") {
    sportspad = true;
  }

  //Sports Pad Soccer (Japan)
  if(hash == "4b856123cd090d584af0a6a94ac0a341aa439562a7d8d9e0669999a292e32309") {
    sportspad = true;
  }

  //Taito Chase H.Q. (Europe)
  if(hash == "174f4a723e0ece21853d622ca0cd68f94000d8ef7c41f4e7d270944c85841cf5") {
    region = "PAL";
  }

  //Woody Pop - Shinjinrui no Block Kuzushi (Japan)
  if(hash == "5cde2716a76d16ee8b693dfe8f08da45d6f76faa945f87657fea9c5cfa64ae34") {
    paddle = true;
    region = "NTSC-J";
  }

  //Xenon 2 - Megablast (Europe)
  if(hash == "c91b07d60f032aee7173faf40fce2537994c8065b664f7d9e940a3891f4b691f") {
    region = "PAL";
  }

  //Xenon 2 - Megablast (Europe) (Rev 1)
  if(hash == "59f4878cf9f075ce7c1d13b29f22061e1d978a66344d889a29de70e5a9b2b6cc") {
    region = "PAL";
  }

  //Ys (Japan)
  if(hash == "816ddb63d3aa30e7c4fc0efb706ff5e66f4b6f17cf331d5fc97f6681d1905a61") {
    //relies on SMS1 VDP bug
    region = "NTSC-J";
  }

  //4pak
  //===========

  //4 PAK All Action (Australia)
  if(hash == "fe2ef2e550b5ca4ecf81786cda858e0f3e537dd3cdf70316c8453320ea4e6957") {
    board  = "pak4";
    region = "PAL";
    ram    = 0;
  }

  //Hicom
  //===========

  //The Best Game Collection - Hang On + Pit Pot + Spy vs Spy (Korea)
  if(hash == "11ef490394ad30bfa795295f0195177f7ac92153521a97dc3d9a817912cd185c") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection - Great Baseball + Great Soccer + Super Tennis (Korea)
  if(hash == "dcd0195f5e9eef77c5ffd23413b54206daa8eacaf9adb41618c16b43c9bf3eb3") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection - Teddy Boy Blues + Pit-Pot + Astro Flash (Korea)
  if(hash == "5e8dba358c6e5ced67d6fd6d3c8e3124ac6bcde73ae5b1b89fe31d5733451fb0") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection - Teddy Boy Blues + Great Soccer + Comical Machine Gun Joe (Korea)
  if(hash == "bee055caea0282a505afd08452000aca3a46ee7a2a7ed1f7cfa09b43580a8431") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection - Ghost House + Teddy Boy Blues + Seishun Scandal (Korea)
  if(hash == "395cf05e2eabc7d47f57e501239f60d35933f450b2205895d25f380ae4b23a7c") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection - Satellite-7 + Great Baseball + Seishun Scandal (Korea)
  if(hash == "6f09849fa4f5904cc9554c15b4522da49160cf99f81895c1df80d3b4bfc0f033") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection (Korea, 8 in 1 Ver. A)
  if(hash == "572dfcc5ade1251c38b729ee86d78e37fde29d948d19f2292aeff32685db9129") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection (Korea, 8 in 1 Ver. B)
  if(hash == "6b829e41da9b7a0f2f0b0fcdbd644e311178ee7958e69ea6a1590dd4794a2834") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //The Best Game Collection (Korea, 8 in 1 Ver. C)
  if(hash == "addf8f41dfb2698cb4e5d01bdff29bfda898d099e651c08d7536eb9e2bb14f5e") {
    board  = "Hicom";
    region = "PAL";
    ram    = 0;
  }

  //Codemasters
  //===========

  //Cosmic Spacehead (Europe)
  if(hash == "1377189c097116fd49f9298368fd3138a28fbe48a08ff7b097d9360ac891bb9b") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Dinobasher Starring Bignose the Caveman (Europe) (Prototype)
  if(hash == "127b14aa1f61a092ce5a70afa63527c039a5125b904cfa3a860c7c713f2e3a24") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //The Excellent Dizzy Collection (Europe, USA, prototype)
  if(hash == "f9cfcad94d7fa8a863d7b9fbeda5ed02e29787f3b56bc6114aea900c2bb44831") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Fantastic Dizzy (Europe)
  if(hash == "f0b9f293ec534f38e1028016e7e8bcb56a96591faa88d6e8ec5cab2bd5abfb3b") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }
  
  //Micro Machines (Europe)
  if(hash == "cfbc86deccdcd31b64f886b1c344cc017e39c665928820bdfa16ae65d281057a") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Korean No-Bank Mapper
  //=====

  //FA Tetris (Korea)
  if(hash == "46d86dd5782b248430cd84e9ba5b4fc68abec6472213bb898b2dbb213b4debcf") {
    board  = "Korea_NB";
    region = "NTSC-J";
    ram    = 0;
  }
  
  //Sky Jaguar
  if(hash == "6b154d9fde7c8c6f1cf45adc50bf4727197746bb30c25f2f3f65874cc85cdf3e") {
    board  = "Korea_NB";
    region = "NTSC-J";
    ram    = 0;
  }
  
  //Sky Jaguar [Clover]
  if(hash == "05462f928c1b39716dea4b230e2ed36b923e9fff47b81fa47c7afdce7cb57e49") {
    board  = "Korea_NB";
    region = "NTSC-J";
    ram    = 0;
  }

  //Flashpoint (Korea)
  // TODO: NOT WORKING!
  if(hash == "c6d945e79a22cc6da9ed6937d6a93a6df64835f1a577454e1c90b03e7d245186") {
    board  = "Korea_NB";
    region = "NTSC-J";
    ram    = 0;
  }

  //Xyzolog (Korea)
  if(hash == "dfb4cf3d3e793f09c683965b5fbc1c1f4d8b580326fc08ecab093275e0aeb537") {
    board  = "Korea_NB";
    region = "NTSC-J";
    ram    = 0;
  }


  //Korea
  //=====

  //Jang Pung II (Korea)
  if(hash == "49df982e7da155d54f5d99aea338d0ada262b28ed6033ff1f88146b976d8b312") {
    board  = "Korea";
    region = "NTSC-J";
    ram    = 0;
  }

  //Jang Pung 3 (Korea)
  if(hash == "ef489ab1a6215957309dff64e672b9a5b66de3826841e523f9d2fea412c6911d") {
    board  = "Korea";
    region = "NTSC-J";
    ram    = 0;
  }

  //Dallyeora Pigu-Wang (Korea)
  if(hash == "c92b7f13bfa2d5bcd57ac0bbeb7f0b0218bbac6430b51f64d9608ee26a3fc3d2") {
    board  = "Korea";
    region = "NTSC-J";
    ram    = 0;
  }
  
/* // MAME says it uses the Korea mapper but the whole game breaks with it? (Verify whats correct)
  //Hong Kil Dong (Korea) 
  if(hash == "b190090854b109ce69e1255dd189e8e985a3339bc083b102c4af73a2d20ca6d7") {
    board  = "Korea";
    region = "NTSC-J";
    ram    = 0;
  }
*/

  //Sangokushi 3 (Korea)
  if(hash == "db3773c81e9c3975f6ba7a6dd06df25b5d5210c69a6d1f13a0b7d5c92b4e0094") {
    board  = "Korea";
    region = "NTSC-J";
    ram    = 0;
  }

  //Super Boy II (Korea)
  if(hash == "4dcec910ed8ff1f0d105457bedaf566e188d40b09efaaf35f28a56d4c5675555") {
    board  = "Korea";
    region = "NTSC-J";
    ram    = 0;
  }
  
  //Zemina / Nemesis
  //===
  //Nemesis (Korea)
  if(hash == "c5b5126b0fdcacc70f350c3673f8d081b8d59d2db251ef917bf3ca8dd3d63430") {
    board  = "Zemina_Nemesis";
    region = "NTSC-J";
    ram    = 0;
  }

  //Zemina
  //===
  //Super Boy 3
  
  //Nemesis 2 (Korea)
  if(hash == "52d0772347e1bd73eea1a31f19400a4e24b43fb148b265ace9018ce2b677b3e4") {
    board  = "Zemina";
    region = "NTSC-J";
    ram    = 0;
  }
  
  //Penguin Adventure (Korea)
  if(hash == "17958bcf247f6d18b616c519cc1466f7250901ada61831890ca58767fbfbb4b8") {
    board  = "Zemina";
    region = "NTSC-J";
    ram    = 0;
  }

  //Street Master (Korea)
  if(hash == "cad9e768d6c7a1f781f2bf6c13ceab9142135d36fadab8db4b77edc9ae926be7") {
    board  = "Zemina";
    region = "NTSC-J";
    ram    = 0;
  }

  //Cyborg Z (Korea) 
  if(hash == "685530e434c9d78da8441475f795591210873622622b7f3168348ce08abc7f8e") {
    board  = "Zemina";
    region = "NTSC-J";
    ram    = 0;
  }

  //Knightmare II: The Maze of Galious (Korea)
  if(hash == "a906127be07469e6083ef6eb9657f5ece6ffe65b3cf793a0423959fc446e2799") {
    board  = "Zemina";
    region = "NTSC-J";
    ram    = 0;
  }

  //Wonsiin (Korea)
  if(hash == "ee22d7b7fcd59e81ee6fda7b82356883d9c3d8ea3c99e6591487d1f28d569073") {
    board  = "Zemina";
    region = "NTSC-J";
    ram    = 0;
  }
  //F-1 Spirit: The Way to Formula-1 (Korea)
  if(hash == "42dbc2ccf30a8d8e6d1d64fd0abb98d2f29b06da2113cedb9ed15af57fd9cfe0") {
    board  = "Zemina";
    region = "NTSC-J";
    ram    = 0;
  }
  
  //Janggun
  //=======

  //Janggun-ui Adeul (Korea)
  if(hash == "e3c19e14a934f0995a046b8fa9d5c8db53fc02979273e747a5083b45996861c4") {
    board  = "Janggun";
    region = "NTSC-J";
    ram    = 0;
  }
  
  //Hap2000
  //=======

  //2 Hap in 1 - David-2 ~ Moai-ui bomul (KR)
  if(hash == "cf6aa5727748f7042ff7942972a27bed015e1e30976606adaad6f8ef0ec214b6") {
    board  = "Hap2000";
    region = "NTSC-J";
    ram    = 0;
  }
  
  
  //Korean 188 in 1
  // TODO: Game 6 and 13 do not work, posibly others also fail.
  //=======
  
  //Not public dump
  // 128 Hap (KR)

  //Game Mo-eumjip 188 Hap (Korea) (v0)
  if(hash == "f6fa88b8f0396e1bda26e00fd8427effc26aaabed6f811aef68efec32b4c4df3") {
    board  = "K188in1";
    region = "NTSC-J";
    ram    = 0;
  }
  //Game Mo-eumjip 188 Hap (Korea) (v1)
  if(hash == "e77553e86a0359c45e5b8250666ea402b0ae0d989b7fce0c28b42954cb5e106a") {
    board  = "K188in1";
    region = "NTSC-J";
    ram    = 0;
  }
  

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  s +={"  board:  ", board, "\n"};
  if(paddle) s += "  paddle\n";
  if(sportspad) s += "  sportspad\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  if(ram) {
  s += "    memory\n";
  s += "      type: RAM\n";
  s +={"      size: 0x", hex(ram), "\n"};
  s += "      content: Save\n";
  }
  return s;
}
