struct SufamiTurbo : Cartridge {
  auto name() -> string override { return "Sufami Turbo"; }
  auto extensions() -> vector<string> override { return {"st"}; }
  auto pak(string location) -> shared_pointer<vfs::directory> override;
  auto rom(string location) -> vector<u8> override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};

auto SufamiTurbo::pak(string location) -> shared_pointer<vfs::directory> {
  if(auto pak = Media::pak(location)) return pak;
  if(auto rom = Media::read(location)) {
    auto pak = shared_pointer{new vfs::directory};
    pak->append("manifest.bml", Cartridge::manifest(rom, location));
    pak->append("program.rom",  rom);
    return pak;
  }
  return {};
}

auto SufamiTurbo::rom(string location) -> vector<u8> {
  vector<u8> data;
  append(data, {location, "program.rom"});
  return data;
}

auto SufamiTurbo::heuristics(vector<u8>& data, string location) -> string {
  if(data.size() < 0x20000) return {};

  u32 romSize = data[0x36] * 128_KiB;
  u32 ramSize = data[0x37] *   2_KiB;

  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
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
