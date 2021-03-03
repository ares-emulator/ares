struct SufamiTurbo : Cartridge {
  auto name() -> string override { return "Sufami Turbo"; }
  auto extensions() -> vector<string> override { return {"st"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto SufamiTurbo::load(string location) -> shared_pointer<vfs::directory> {
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
  pak->setAttribute("title", document["game/title"].string());
  //todo: update boards database to use title: instead of label:
  if(!document["game/title"]) {
    pak->setAttribute("title", document["game/label"].string());
  }

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Media::load(location, pak, node, ".ram");
  }

  return pak;
}

auto SufamiTurbo::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Media::save(location, pak, node, ".ram");
  }

  return true;
}

auto SufamiTurbo::heuristics(vector<u8>& data, string location) -> string {
  if(data.size() < 0x20000) return {};

  u32 romSize = data[0x36] * 128_KiB;
  u32 ramSize = data[0x37] *   2_KiB;

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  title: ", Media::name(location), "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";

  if(ramSize) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(ramSize), "\n"};
    s += "      content: Save\n";
  }

  return s;
}
