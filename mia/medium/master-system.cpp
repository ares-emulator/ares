struct MasterSystem : Cartridge {
  auto name() -> string override { return "Master System"; }
  auto extensions() -> vector<string> override { return {"ms", "sms"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto MasterSystem::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  } else {
    return false;
  }

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("board",  document["game/board" ].string());
  pak->setAttribute("title",  document["game/title" ].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }

  return true;
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

  //Region
  //======

  //PGA Tour Golf (Europe)
  if(hash == "a3856f0d15511f7a6d48fa6c47da2c81adf372e6a04f83298a10cd45b6711530") {
    region = "PAL";
  }

  //Ys (Japan)
  if(hash == "816ddb63d3aa30e7c4fc0efb706ff5e66f4b6f17cf331d5fc97f6681d1905a61") {
    //relies on SMS1 VDP bug
    region = "NTSC-J";
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

  //Korea
  //=====

  //C_So! (Korea)
  if(hash == "60716197f2643a5c87e38d26f32f370e328e740aaaea879b521986c31f8cdded") {
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

  //FA Tetris (Korea)
  if(hash == "46d86dd5782b248430cd84e9ba5b4fc68abec6472213bb898b2dbb213b4debcf") {
    board  = "Korea";
    region = "NTSC-J";
    ram    = 0;
  }

  //MSX
  //===

  //Cyborg Z (Korea)
  if(hash == "685530e434c9d78da8441475f795591210873622622b7f3168348ce08abc7f8e") {
    board  = "MSX";
    region = "NTSC-J";
    ram    = 0;
  }

  //F-1 Spirit: The Way to Formula-1 (Korea)
  if(hash == "42dbc2ccf30a8d8e6d1d64fd0abb98d2f29b06da2113cedb9ed15af57fd9cfe0") {
    board  = "MSX";
    region = "NTSC-J";
    ram    = 0;
  }

  //Knightmare II: The Maze of Galious (Korea)
  if(hash == "a906127be07469e6083ef6eb9657f5ece6ffe65b3cf793a0423959fc446e2799") {
    board  = "MSX";
    region = "NTSC-J";
    ram    = 0;
  }

  //Nemesis (Korea)
  if(hash == "c5b5126b0fdcacc70f350c3673f8d081b8d59d2db251ef917bf3ca8dd3d63430") {
    board  = "MSX#A";
    region = "NTSC-J";
    ram    = 0;
  }

  //Nemesis 2 (Korea)
  if(hash == "52d0772347e1bd73eea1a31f19400a4e24b43fb148b265ace9018ce2b677b3e4") {
    board  = "MSX";
    region = "NTSC-J";
    ram    = 0;
  }

  //Penguin Adventure (Korea)
  if(hash == "17958bcf247f6d18b616c519cc1466f7250901ada61831890ca58767fbfbb4b8") {
    board  = "MSX";
    region = "NTSC-J";
    ram    = 0;
  }

  //Street Master (Korea)
  if(hash == "cad9e768d6c7a1f781f2bf6c13ceab9142135d36fadab8db4b77edc9ae926be7") {
    board  = "MSX";
    region = "NTSC-J";
    ram    = 0;
  }

  //Wonsiin (Korea)
  if(hash == "ee22d7b7fcd59e81ee6fda7b82356883d9c3d8ea3c99e6591487d1f28d569073") {
    board  = "MSX";
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

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  s +={"  board:  ", board, "\n"};
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
