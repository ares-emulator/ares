struct GameGear : Cartridge {
  auto name() -> string override { return "Game Gear"; }
  auto extensions() -> vector<string> override { return {"gg"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto GameGear::load(string location) -> bool {
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
  pak->setAttribute("ms",     (bool)document["game/board/ms"]);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }

  return true;
}

auto GameGear::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }

  return true;
}

auto GameGear::analyze(vector<u8>& rom) -> string {
  string hash   = Hash::SHA256(rom).digest();
  string board  = "Sega";
  string region = "NTSC-J, NTSC-U, PAL";  //database required to detect region
  u32 ram       = 32_KiB;                 //database required to detect RAM size
  bool ms       = 0;                      //database required to detect Master System mode

  //Master System
  //=============

  //Castle of Illusion Starring Mickey Mouse (USA, Europe)
  if(hash == "068fc6eaf728b3cd17c6fc5320c955deb0cd3b36343810470fe30c5a3661d0d3") {
    region = "NTSC-U, PAL";
    ms     = 1;
  }

  //Jang Pung II (Korea)
  if(hash == "a685ad4118edd2fe6fb8ccc4bfc0c9ceac2f6affc3b5d93cc29afb2c9604d5ee") {
    region = "NTSC-J";
    ms     = 1;
  }

  //Olympic Gold (Japan, USA)
  if(hash == "8c591ccc49c7806aeeab67c4c82babec29ee49c9f5b179fe5f811a5c15f0b17e") {
    region = "NTSC-J, NTSC-U";
    ms     = 1;
  }

  //Olympic Gold (Europe)
  if(hash == "7a842949ec9f8e8f564b56e550ff146c3d3999adbe598c94f751c629d4346f86") {
    region = "PAL";
    ms     = 1;
  }

  //Out Run Europa (USA)
  if(hash == "a507760d4526e21056fcb97a981fb84da23ff371c11ffd5980f65d1db88953d6") {
    region = "NTSC-U";
    ms     = 1;
  }

  //Out Run Europa (Europe)
  if(hash == "eed5943b18ca2ce7d2fc4db1123da5c330ae3a07258741ded6c4333202b878fb") {
    region = "PAL";
    ms     = 1;
  }

  //Predator 2 (USA, Europe)
  if(hash == "6ebae288656b12612ded3aceda7bd24844341cd536bdaf767b61e4e8911bb369") {
    region = "NTSC-U, PAL";
    ms     = 1;
  }

  //Prince of Persia (USA, Europe)
  if(hash == "54a9657e6c489ce03b8e9ceb0096152c211b356398541b84c2480ea3297f7fc2") {
    region = "NTSC-U, PAL";
    ms     = 1;
  }

  //Prince of Persia (USA, Europe) (Beta)
  if(hash == "3bd19b204eeb6f00d2fbb8ad31db1118f0c8ef8658246ad361ba9545481eee48") {
    region = "NTSC-U, PAL";
    ms     = 1;
  }

  //Rastan Saga (Japan)
  if(hash == "d8756c6c8274b3eb049d301ccfdcc15fb0d7cfae7f021117caebe97390867272") {
    region = "NTSC-J";
    ms     = 1;
  }

  //R.C. Grand Prix (USA, Europe)
  if(hash == "faee18f47f5ddffa3f741e40d5a1b5e94b6ea716c129824cd0fb4cf22d0bf669") {
    region = "NTSC-U, PAL";
    ms     = 1;
  }

  //Street Hero (USA) (Proto 1)
  if(hash == "ca16eb3a748ad5139fa591db3d52ecac782c7e0b043af5522858c03df169c6bd") {
    region = "NTSC-U";
    ms     = 1;
  }

  //Street Hero (USA) (Proto 2)
  if(hash == "ee84e6a87df1a668e5501deeb619a03172795b468fd5b34ec902b62e73dd5459") {
    region = "NTSC-U";
    ms     = 1;
  }

  //Super Kick Off (Japan)
  if(hash == "0ebc52e06a644e7f31802bee9f1b4b41b747790f1e841a8417bb561e475e58d3") {
    region = "NTSC-J";
    ms     = 1;
  }

  //WWF Wrestlemania Steel Cage Challenge (Europe)
  if(hash == "71d8d0769bf9c7d9339ead0319062d85a0e19411a17a938f58678b0b8efa7132") {
    region = "PAL";
    ms     = 1;
  }

  //Codemasters
  //===========

  //CJ Elephant Fugitive (Europe)
  if(hash == "bab8896529fdbab1d85c16048582e3639e2f85e12875862ce76e7f862706a52d") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Cosmic Spacehead (Europe)
  if(hash == "d5f16bf7a21fb6a6c1ebd6196aa6c1327275940613e877ccb4c1366eab66481e") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Dropzone (Europe)
  if(hash == "8b949055eead8fc5fa9ae9c6530373cac77ffd9be7159090f4eb057919915a4f") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Ernie Els Golf (Europe)
  if(hash == "b1efbd983423560707c0ca31ea9780c31fbb4f3d37a77ee1064453ad778e010d") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 8_KiB;
  }

  //Excellent Dizzy Collection, The (Europe)
  if(hash == "7ad6a32bbc270af48d55c6650bdd22cf4050ed50a446334de952b18490798bf5") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
    ms     = 1;
  }

  //Fantastic Dizzy (Europe)
  if(hash == "cf9e92667b4d653996a8b678998211a31536410a39e4f64ca535feea5450ad0c") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
    ms     = 1;
  }

  //Micro Machines (Europe)
  if(hash == "d842c0408ef1b085bb0ac0aaed4f53f9dd5659a11a6e12df12adbe0fec5c46c8") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Micro Machines 2: Turbo Tournament (Europe)
  if(hash == "8c832bce275ebb4011a65875b38402c00fa5df90534560f3977aed51fe06d00e") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //Pete Sempras Tennis (Europe)
  if(hash == "3fa10454535274c67f702ddc9c47f587f6895996fa35dedaaad97b7ffe2e10bc") {
    board  = "Codemasters";
    region = "PAL";
    ram    = 0;
  }

  //S.S. Lucifer: Man Overboard! (Europe)
  if(hash == "3cbd347c1a584fbaa37f3f53797824ce7532e9d12fcb46b2efbc51bfb5f2d83e") {
    board  = "Codemasters";
    region = "PAL";
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
  if(ms)
  s += "    ms";
  return s;
}
