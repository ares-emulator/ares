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
  //##FIX## Although not advertised as such, all MegaLD games tested so far are in fact region free, and will work
  //under either a US or Japan bios. Additionally, for all games tested so far which were released in both the US and
  //Japan, they will derive their default language based on the region of the bios. Due to this, and also considering
  //the current lack of available rips for all regions, we do not currently lock the games into one region, and
  //instead let the user region preferences determine which region will be selected.
//  if(mmiArchive.region() == "U") region = "NTSC-U";
//  if(mmiArchive.region() == "J") region = "NTSC-J";

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
