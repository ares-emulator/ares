auto PlayStation::manifest(vector<uint8_t> sector, string location) -> string {
  string text;
  text.resize(sector.size());
  memory::copy(text.get(), sector.data(), sector.size());

  string region;
  if(text.find("Sony Computer Entertainment Inc."      )) region = "NTSC-J";
  if(text.find("Sony Computer Entertainment Amer"      )) region = "NTSC-U";
  if(text.find("Sony Computer Entertainment of America")) region = "NTSC-U";
  if(text.find("Sony Computer Entertainment Euro"      )) region = "PAL";
  if(!region) return {};  //audio CD or non-PlayStation CD

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", region, "\n"};
  return s;
}

auto PlayStation::manifest(string location) -> string {
  if(!location.endsWith(".exe")) return CompactDisc::manifest(location);

  auto exe = file::read(location);
  if(exe.size() < 2048) return {};
  if(memory::compare(exe.data(), "PS-X EXE", 8)) return {};

  string s;
  s += "game\n";
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
  s +={"  region: ", "NTSC-U", "\n"};
  s +={"  executable\n"};
  return s;
}

auto PlayStation::cdFromExecutable(string location) -> vector<uint8_t> {
  auto exe = file::read(location);
  if(exe.size() < 2048) return {};
  if(memory::compare(exe.data(), "PS-X EXE", 8)) return {};
  vector<uint8_t> cd;
  return cd;
}
