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

  //attempt to apply BPS patch if one was found
  if(patch) {
    if(auto output = Beat::Single::apply(memory, patch)) {
      memory = move(*output);
    }
  }

  return memory;
}

auto Media::manifest(string location) -> string {
  if(auto pak = load(location)) {
    if(auto fp = pak->read("manifest.bml")) {
      return fp->reads();
    }
  }
  return {};
}

auto Media::load(shared_pointer<vfs::directory> pak, string location, Markup::Node node, string extension) -> bool {
  string name;
  if(auto architecture = node["architecture"].string()) name.append(architecture, ".");
  name.append(node["content"].string(), ".");
  name.append(node["type"].string());
  name.downcase();
  auto size = node["size"].natural();

  string saveLocation;
  if(auto path = mia::saveLocation()) {
    saveLocation = {path, this->name(), "/", Location::prefix(location), extension};
  } else if(file::exists(location)) {
    saveLocation = {Location::notsuffix(location), extension};
  } else if(directory::exists(location)) {
    saveLocation = {location, name};
  } else {
    return false;
  }

  auto save = memory::allocate<u8>(size, 0xff);
  if(auto fp = file::open(saveLocation, file::mode::read)) {
    fp.read({save, min(fp.size(), size)});
  }
  pak->append(name, {save, size});
  return true;
}

auto Media::save(shared_pointer<vfs::directory> pak, string location, Markup::Node node, string extension) -> bool {
  string name;
  if(auto architecture = node["architecture"].string()) name.append(architecture, ".");
  name.append(node["content"].string(), ".");
  name.append(node["type"].string());
  name.downcase();
  auto size = node["size"].natural();

  string saveLocation;
  if(auto path = mia::saveLocation()) {
    saveLocation = {path, this->name(), "/", Location::prefix(location), extension};
  } else if(file::exists(location)) {
    saveLocation = {Location::notsuffix(location), extension};
  } else if(directory::exists(location)) {
    saveLocation = {location, name};
  } else {
    return false;
  }

  if(auto save = pak->read<vfs::memory>(name)) {
    if(auto fp = file::open(saveLocation, file::mode::write)) {
      fp.write({save->data(), save->size()});
      return true;
    }
  }
  return false;
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
