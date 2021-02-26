auto PlayStation::pak(string location) -> shared_pointer<vfs::directory> {
  if(auto pak = Media::pak(location)) return pak;
  auto pak = shared_pointer{new vfs::directory};
  pak->append("manifest.bml", manifest(location));
  if(location.iendsWith(".cue")) {
    pak->append("cd.rom", vfs::cdrom::open(location));
  }
  if(location.iendsWith(".exe")) {
    pak->append("program.exe", vfs::disk::open(location, vfs::file::mode::read));
  }
  return pak;
}

auto PlayStation::rom(string location) -> vector<u8> {
  vector<u8> data;
  append(data, {location, "cd.rom"});
  append(data, {location, "program.exe"});
  return data;
}

auto PlayStation::manifest(string location) -> string {
  if(location.iendsWith(".cue")) {
    auto sector = readDataSectorCUE(location, 4);
    if(!sector) return CompactDisc::manifest(location);

    string text;
    text.resize(sector.size());
    memory::copy(text.get(), sector.data(), sector.size());

    string region;
    if(text.find("Sony Computer Entertainment Inc."      )) region = "NTSC-J";
    if(text.find("Sony Computer Entertainment Amer"      )) region = "NTSC-U";
    if(text.find("Sony Computer Entertainment of America")) region = "NTSC-U";
    if(text.find("Sony Computer Entertainment Euro"      )) region = "PAL";
    if(!region) return CompactDisc::manifest(location);

    string s;
    s += "game\n";
    s +={"  name:   ", Media::name(location), "\n"};
    s +={"  label:  ", Media::name(location), "\n"};
    s +={"  region: ", region, "\n"};
    return s;
  }

  if(location.iendsWith(".exe")) {
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

  return {};
}

//todo: not implemented yet
auto PlayStation::cdFromExecutable(string location) -> vector<u8> {
  auto exe = file::read(location);
  if(exe.size() < 2048) return {};
  if(memory::compare(exe.data(), "PS-X EXE", 8)) return {};
  vector<u8> cd;
  return cd;
}
