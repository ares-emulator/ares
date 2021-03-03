struct MSX : Cartridge {
  auto name() -> string override { return "MSX"; }
  auto extensions() -> vector<string> override { return {"msx"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto MSX::load(string location) -> shared_pointer<vfs::directory> {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return {};

  auto pak = shared_pointer{new vfs::directory};
  auto manifest = Cartridge::manifest(rom, location);
  auto document = BML::unserialize(manifest);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board",  document["game/board"].string());
  return pak;
}

auto MSX::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  return true;
}

auto MSX::heuristics(vector<u8>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  title:  ", Media::name(location), "\n"};
  s += "  region: NTSC\n";  //database required to detect region
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  return s;
}
