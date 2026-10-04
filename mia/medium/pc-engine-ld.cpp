struct PCEngineLD : LaserDisc {
  auto name() -> string override { return "LaserActive (NEC PAC)"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto PCEngineLD::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  if(!mmiArchive.open(location)) return invalidROM;
  if(!mmiArchive.media().size()) return invalidROM;
  if((mmiArchive.system() != "LDROM2") && (mmiArchive.system() != "LD") && (mmiArchive.system() != "CDROM2") && (mmiArchive.system() != "SuperCDROM2") && (mmiArchive.system() != "ArcadeCDROM2") && (mmiArchive.system() != "CD")) {
    LoadResult result(wrongMediaType);
    result.mediaType = mmiArchive.system();
    return result;
  }

  this->location = location;
  this->manifest = mmiArchive.manifest();
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  string medium = "";
  for(auto& media : mmiArchive.media()) {
    if(medium) medium += ", ";
    medium += media.name;
  }

  string region = "NTSC-J, NTSC-U";

  pak = std::make_shared<vfs::directory>();
  if(mmiArchive.catalogId().length() > 0) {
    pak->setAttribute("title", { mmiArchive.name(), " [", mmiArchive.catalogId(), "]" });
  } else {
    pak->setAttribute("title", mmiArchive.name());
  }
  pak->setAttribute("region", region);
  pak->setAttribute("location", location);
  pak->setAttribute("system", mmiArchive.system());
  pak->setAttribute("medium", medium);
  pak->append("manifest.bml", manifest);

  for(auto& media : mmiArchive.media()) {
    for(auto& stream : media.streams) {
      if(stream.name == "DigitalAudio") {
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
