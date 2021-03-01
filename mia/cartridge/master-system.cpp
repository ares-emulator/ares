struct MasterSystem : Cartridge {
  auto name() -> string override { return "Master System"; }
  auto extensions() -> vector<string> override { return {"ms", "sms"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto MasterSystem::load(string location) -> shared_pointer<vfs::directory> {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  } else {
    return {};
  }

  auto pak = shared_pointer{new vfs::directory};
  auto manifest = Cartridge::manifest(rom, location);
  auto document = BML::unserialize(manifest);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Media::load(pak, location, node, ".ram");
  }

  return pak;
}

auto MasterSystem::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Media::save(pak, location, node, ".ram");
  }

  return true;
}

auto MasterSystem::heuristics(vector<u8>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s += "  region: NTSC-J, NTSC-U, PAL\n";  //database required to detect region
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x8000\n";
  s += "      content: Save\n";
  return s;
}
