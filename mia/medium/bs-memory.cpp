struct BSMemory : Cartridge {
  auto name() -> string override { return "BS Memory"; }
  auto extensions() -> vector<string> override { return {"bs"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto BSMemory::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
    append(rom, {location, "program.flash"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return false;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = Medium::manifestDatabase(sha256);
  if(!manifest) manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title", document["game/title"].string());
  pak->append("manifest.bml", manifest);
  if(document["game/board/memory(type=ROM)"]) {
    pak->append("program.rom", rom);
  }
  if(document["game/board/memory(type=Flash)"]) {
    pak->append("program.flash", rom);
    Pak::load("program.flash", ".sav");
  }

  return true;
}

auto BSMemory::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(document["game/board/memory(type=Flash)"]) {
    Pak::save("program.flash", ".sav");
  }

  return true;
}

auto BSMemory::analyze(vector<u8>& rom) -> string {
  if(rom.size() < 0x8000) return {};

  auto type = "Flash";
  auto digest = Hash::SHA256(rom).digest();

  //Same Game: Chara Cassette
  if(digest == "80c34b50817d58820bc8c88d2d9fa462550b4a76372e19c6467cbfbc8cf5d9ef") type = "ROM";

  //Same Game: Chara Data Shuu
  if(digest == "859c7f7b4771d920a5bdb11f1d247ab6b43fb026594d1062f6f72d32cd340a0a") type = "ROM";

  //SD Gundam G Next: Unit & Map Collection
  if(digest == "c92a15fdd9b0133f9ea69105d0230a3acd1cdeef98567462eca86ea02a959e4e") type = "ROM";

  string s;
  s += "game\n";
  s +={"  name:  ", Medium::name(location), "\n"};
  s +={"  title: ", Medium::name(location), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s +={"      type: ", type, "\n"};
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
