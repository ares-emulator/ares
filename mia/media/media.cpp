auto Media::construct() -> void {
  database = BML::unserialize(file::read(locate({"Database/", name(), ".bml"})));
  pathname = {Path::user(), "Emulation/", name(), "/"};
}

auto Media::read(string location) -> vector<u8> {
  vector<u8> memory;
  vector<u8> patch;

  //support game ROMs
  if(file::exists(location)) {
    //support BPS patches next to the file
    patch = file::read({Location::notsuffix(location), ".bps"});
    if(location.iendsWith(".zip")) {
      Decode::ZIP archive;
      if(archive.open(location)) {
        auto suffixes = extensions();
        for(auto& file : archive.file) {
          auto suffix = Location::suffix(file.name).trimLeft(".", 1L).downcase();
          if(suffixes && !suffixes.find(suffix)) continue;
          memory = archive.extract(file);
          break;
        }
        //support BPS patches inside the ZIP archive
        for(auto& file : archive.file) {
          auto suffix = Location::suffix(file.name).downcase();
          if(suffix != ".bps") continue;
          patch = archive.extract(file);
          break;
        }
      }
    } else {
      memory = file::read(location);
    }
  }

  //support game paks
  if(directory::exists(location)) {
    memory = rom(location);
    patch = file::read({location, "patch.bps"});
  }

  //attempt to apply BPS patch if one was found
  if(patch) {
    if(auto output = Beat::Single::apply(memory, patch)) {
      memory = move(*output);
    }
  }

  return memory;
}

//handles game paks
auto Media::pak(string location) -> shared_pointer<vfs::directory> {
  if(directory::exists(location)) {
    auto pak = shared_pointer{new vfs::directory};
    //todo: handle recursive directories for eg "msu1/*"
    for(auto& filename : directory::files(location)) {
      pak->append(filename, vfs::disk::open({location, filename}, vfs::file::mode::read));
    }
    //generate manifest for gamepak dynamically if it is not already present
    if(!pak->find("manifest.bml")) {
      pak->append("manifest.bml", manifest(location));
    }
    return pak;
  }
  return {};
}

auto Media::name(string location) const -> string {
  return Location::prefix(location);
}

auto Media::append(vector<u8>& output, string filename) -> bool {
  if(!file::exists(filename)) return false;
  auto input = file::read(filename);
  auto size = output.size();
  output.resize(size + input.size());
  memory::copy(output.data() + size, input.data(), input.size());
  return true;
}
