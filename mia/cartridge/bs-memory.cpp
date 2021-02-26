struct BSMemory : Cartridge {
  auto name() -> string override { return "BS Memory"; }
  auto extensions() -> vector<string> override { return {"bs"}; }
  auto pak(string location) -> shared_pointer<vfs::directory> override;
  auto rom(string location) -> vector<u8> override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto BSMemory::pak(string location) -> shared_pointer<vfs::directory> {
  if(auto pak = Media::pak(location)) return pak;
  if(auto rom = Media::read(location)) {
    auto pak = shared_pointer{new vfs::directory};
    auto manifest = Cartridge::manifest(rom, location);
    auto document = BML::unserialize(manifest);
    pak->append("manifest.bml", manifest);
    if(document["game/board/memory(type=ROM)"]) {
      pak->append("program.rom", rom);
    }
    if(document["game/board/memory(type=Flash)"]) {
      pak->append("program.flash", rom);
    }
    return pak;
  }
  return {};
}

auto BSMemory::rom(string location) -> vector<u8> {
  vector<u8> data;
  append(data, {location, "program.rom"});
  append(data, {location, "program.flash"});
  return data;
}

auto BSMemory::heuristics(vector<u8>& data, string location) -> string {
  if(data.size() < 0x8000) return {};

  auto type = "Flash";
  auto digest = Hash::SHA256(data).digest();

  //Same Game: Chara Cassette
  if(digest == "80c34b50817d58820bc8c88d2d9fa462550b4a76372e19c6467cbfbc8cf5d9ef") type = "ROM";

  //Same Game: Chara Data Shuu
  if(digest == "859c7f7b4771d920a5bdb11f1d247ab6b43fb026594d1062f6f72d32cd340a0a") type = "ROM";

  //SD Gundam G Next: Unit & Map Collection
  if(digest == "c92a15fdd9b0133f9ea69105d0230a3acd1cdeef98567462eca86ea02a959e4e") type = "ROM";

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s +={"      type: ", type, "\n"};
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
