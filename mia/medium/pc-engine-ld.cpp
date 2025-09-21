struct PCEngineLD : LaserDisc {
  auto name() -> string override { return "PC Engine LD"; }
  auto extensions() -> std::vector<string> override {
#if defined(ARES_ENABLE_CHD)
    return {"cue", "chd", "mmi"};
#else
    return {"cue", "mmi"};
#endif
  }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  //auto analyze(string location) -> string;
};

auto PCEngineLD::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  if (!mmiArchive.open(location)) return invalidROM;
  if (!mmiArchive.media().size()) return invalidROM;

  this->location = location;
  this->manifest = mmiArchive.manifest();
  auto document = BML::unserialize(manifest);
  if (!document) return couldNotParseManifest;

  string medium = "";
  for (auto& media : mmiArchive.media()) {
      if (medium) medium += ", ";
      medium += media.name;
  }

  string region = "NTSC-J, NTSC-U";

  pak = new vfs::directory;
  pak->setAttribute("title", { mmiArchive.name(), " [", mmiArchive.catalogId(), "]" });
  pak->setAttribute("region", region);
  pak->setAttribute("location", location);
  pak->setAttribute("system", mmiArchive.system());
  pak->setAttribute("medium", medium);
  pak->append("manifest.bml", manifest);

  for (auto& media : mmiArchive.media()) {
    for (auto& stream : media.streams) {
      if (stream.name == "DigitalAudio") {
        pak->append(stream.file, vfs::cdrom::open(location, stream.file));
      }
    }
  }

  return successful;
}

auto PCEngineLD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}
