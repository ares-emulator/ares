struct MegaLD : LaserDisc {
  auto name() -> string override { return "Mega LD"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto MegaLD::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  if(!location.iendsWith(".mmi")) return invalidROM;
  if(!mmiArchive.open(location)) return invalidROM;
  if(!mmiArchive.media().size()) return invalidROM;

  this->location = location;
  this->manifest = mmiArchive.manifest();
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  string region = "NTSC-J, NTSC-U";
  if(mmiArchive.region() == "U") region = "NTSC-U";
  if(mmiArchive.region() == "J") region = "NTSC-J";

  string medium = "";
  for(auto& media : mmiArchive.media()) {
    if(medium) medium += ", ";
    medium += media.name;
  }

  pak = new vfs::directory;
  pak->setAttribute("title", {mmiArchive.name(), " [", mmiArchive.catalogId(), "]"});
  pak->setAttribute("region", region);
  pak->setAttribute("location", location);
  pak->setAttribute("system", mmiArchive.system());
  pak->setAttribute("medium", medium);
  pak->append("manifest.bml", manifest);

  for(auto& media : mmiArchive.media()) {
    for(auto& stream : media.streams) {
      if(stream.name == "DigitalAudio") {
        pak->append(stream.file,  vfs::cdrom::open(location, stream.file));
      }
    }
  }

  return successful;
}

auto MegaLD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

