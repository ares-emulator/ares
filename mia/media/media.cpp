auto Media::construct() -> void {
  database = BML::unserialize(file::read(locate({"Database/", name(), ".bml"})));
  pathname = {Path::user(), "Emulation/", name(), "/"};
}

auto Media::read(string location, string suffix) -> vector<u8> {
  if(location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(location)) {
      for(auto& file : archive.file) {
        if(suffix && !file.name.iendsWith(suffix)) continue;
        return archive.extract(file);
      }
    }
  }
  if(suffix) location = {Location::notsuffix(location), suffix};
  return file::read(location);
}

auto Media::location(string location, string suffix) const -> string {
  return {Location::notsuffix(location), suffix};
}

auto Media::name(string location) const -> string {
  return Location::prefix(location);
}
