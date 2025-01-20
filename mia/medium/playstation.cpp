struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto extensions() -> vector<string> override { return {"cue", "chd", "exe"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
  auto cdFromExecutable(string location) -> vector<u8>;
};

auto PlayStation::load(string location) -> LoadResult {
  if(!inode::exists(location)) return LoadResult(romNotFound);

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return LoadResult(couldNotParseManifest);

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("audio", (bool)document["game/audio"]);
  pak->setAttribute("executable", (bool)document["game/executable"]);
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("cd.rom", vfs::disk::open({location, "cd.rom"}, vfs::read));
  }
  if(file::exists(location)) {
    if(location.iendsWith(".cue")) {
      pak->append("cd.rom", vfs::cdrom::open(location));
    }
    if(location.iendsWith(".chd")) {
      pak->append("cd.rom", vfs::cdrom::open(location));
    }
    if(location.iendsWith(".exe")) {
      pak->append("program.exe", vfs::disk::open(location, vfs::read));
    }
  }

  return LoadResult(successful);
}

auto PlayStation::save(string location) -> bool {
  return true;
}

auto PlayStation::analyze(string location) -> string {
  if(location.iendsWith(".cue") || location.iendsWith(".chd")) {
    vector<u8> sector;

    if(location.iendsWith(".cue")) {
      sector = readDataSectorCUE(location, 4);
    } else if (location.iendsWith(".chd")) {
      sector = readDataSectorCHD(location, 4);
    }

    if(!sector) return CompactDisc::manifestAudio(location);

    string text;
    text.resize(sector.size());
    memory::copy(text.get(), sector.data(), sector.size());

    string region;
    if(text.find("Sony Computer Entertainment Inc."      )) region = "NTSC-J";
    if(text.find("Sony Computer Entertainment Amer"      )) region = "NTSC-U";
    if(text.find("Sony Computer Entertainment of America")) region = "NTSC-U";
    if(text.find("Sony Computer Entertainment Euro"      )) region = "PAL";
    if(!region) return CompactDisc::manifestAudio(location);

    string s;
    s += "game\n";
    s +={"  name:   ", Medium::name(location), "\n"};
    s +={"  title:  ", Medium::name(location), "\n"};
    s +={"  region: ", region, "\n"};
    return s;
  }

  if(location.iendsWith(".exe")) {
    auto exe = file::read(location);
    if(exe.size() < 2048) return {};
    if(memory::compare(exe.data(), "PS-X EXE", 8)) return {};

    string s;
    s += "game\n";
    s +={"  name:   ", Medium::name(location), "\n"};
    s +={"  title:  ", Medium::name(location), "\n"};
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
