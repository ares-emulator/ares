struct NeoGeo : Cartridge {
  auto name() -> string override { return "Neo Geo"; }
  auto extensions() -> vector<string> override { return {"ng"}; }
  auto pak(string location) -> shared_pointer<vfs::directory> override;
  auto rom(string location) -> vector<u8> override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto NeoGeo::pak(string location) -> shared_pointer<vfs::directory> {
  if(auto pak = Media::pak(location)) return pak;
  if(auto rom = Media::read(location)) {
    auto pak = shared_pointer{new vfs::directory};
    pak->append("manifest.bml", Cartridge::manifest(rom, location));
    pak->append("program.rom",  rom);
    return pak;
  }
  return {};
}

auto NeoGeo::rom(string location) -> vector<u8> {
  vector<u8> data;
  append(data, {location, "program.rom"});
  return data;
}

auto NeoGeo::heuristics(vector<u8>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
