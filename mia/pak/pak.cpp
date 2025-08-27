auto Pak::create(string name) -> shared_pointer<Pak> {
  auto pak = new Pak;
  pak->pak = new vfs::directory;
  pak->pak->setAttribute("name", name);
  pak->pak->setAttribute("type", "Pak");
  return pak;
}

auto Pak::name(string location) const -> string {
  return Location::prefix(location);
}

auto Pak::read(string location) -> vector<u8> {
  //attempt to match known extensions
  auto extensions = this->extensions();
  for(auto& extension : extensions) extension.prepend("*.");
  auto memory = read(location, extensions);

  //failing that, read whatever exists
  if(!memory) memory = read(location, {"*"});

  return memory;
}

auto Pak::read(string location, vector<string> match) -> vector<u8> {
  vector<u8> memory;
  vector<u8> ips_patch;
  vector<u8> bps_patch;

  if(file::exists(location)) {
    //support IPS or BPS patches next to the file
    bps_patch = file::read({Location::notsuffix(location), ".bps"});
    if (!bps_patch) {
      ips_patch = file::read({Location::notsuffix(location), ".ips"});
    }

    if(location.iendsWith(".zip")) {
      Decode::ZIP archive;
      if(archive.open(location)) {
        for(auto& file : archive.file) {
          for(auto& pattern : match) {
            if(file.name.imatch(pattern)) {
              auto tmp = archive.extract(file);
              memory.resize(tmp.size());
              if(!tmp.empty()) memcpy(memory.data(), tmp.data(), tmp.size());
              break;
            }
          }
          if(memory) break;
        }
        if(memory) {
          //support BPS patches inside the ZIP archive
          for(auto& file : archive.file) {
            if(file.name.imatch("*.bps")) {
              auto tmp = archive.extract(file);
              bps_patch.resize(tmp.size());
              if(!tmp.empty()) memcpy(bps_patch.data(), tmp.data(), tmp.size());
              break;
            } else if (file.name.imatch("*.ips")) {
              auto tmp = archive.extract(file);
              ips_patch.resize(tmp.size());
              if(!tmp.empty()) memcpy(ips_patch.data(), tmp.data(), tmp.size());
              break;
            }
          }
        }
      }
    } else {
      for(auto& pattern : match) {
        if(location.imatch(pattern)) {
          memory = file::read(location);
          break;
        }
      }
    }
  }

  //attempt to apply IPS or BPS patch if one was found, favoring BPS
  if(bps_patch) {
    if(auto output = Beat::Single::apply(memory, bps_patch)) {
      memory = std::move(*output);
    }
  } else if (ips_patch) {
    if (auto output = IPS::apply(memory, ips_patch)) {
      memory = std::move(*output);
    }
  }

  return memory;
}

auto Pak::append(vector<u8>& output, string filename) -> bool {
  if(!file::exists(filename)) return false;
  auto input = file::read(filename);
  auto size = output.size();
  output.resize(size + input.size());
  memory::copy(output.data() + size, input.data(), input.size());
  return true;
}

auto Pak::load(string name, string extension, string location) -> bool {
  if(!pak) return false;
  if(!location) location = this->location;
  if(auto load = pak->write(name)) {
    if(auto memory = file::read(saveLocation(location, name, extension))) {
      if(!load->size()) load->resize(memory.size());
      load->write(memory);
      load->setAttribute("loaded", true);
      return true;
    }
  }
  return false;
}

auto Pak::save(string name, string extension, string location) -> bool {
  if(!pak) return false;
  if(!location) location = this->location;
  if(auto save = pak->read(name)) {
    directory::create(Location::dir(saveLocation(location, name, extension)));
    if(auto fp = file::open(saveLocation(location, name, extension), file::mode::write)) {
      fp.write({save->data(), save->size()});
      return true;
    }
  }
  return true;
}

auto Pak::load(Markup::Node node, string extension, string location) -> bool {
  string name;
  if(auto architecture = node["architecture"].string()) name.append(architecture, ".");
  name.append(node["content"].string(), ".");
  name.append(node["type"].string());
  name.downcase();
  auto size = node["size"].natural();
  pak->append(name, size);
  if(auto fp = pak->write(name)) {
    //pak->append() will allocate memory using 0x00 bytes.
    //but it's more compatible to initialize RAM to 0xff bytes.
    memory::fill<u8>(fp->data(), fp->size(), 0xff);
  }
  return load(name, extension, location);
}

auto Pak::save(Markup::Node node, string extension, string location) -> bool {
  string name;
  if(auto architecture = node["architecture"].string()) name.append(architecture, ".");
  name.append(node["content"].string(), ".");
  name.append(node["type"].string());
  name.downcase();
  auto size = node["size"].natural();
  return save(name, extension, location);
}

auto Pak::saveLocation(string location, string name, string extension) -> string {
  string saveLocation;
  if(auto path = mia::saveLocation()) {
    //if the user has chosen a specific location to save files to ...
    saveLocation = {path, this->saveName(), "/", Location::prefix(location), extension};
  } else if(directory::exists(location)) {
    //if this is a pak ...
    saveLocation = {location, name};
  } else if(file::exists(location)) {
    //if this is a ROM ...
    saveLocation = {Location::notsuffix(location), extension};
  } else {
    //if no path information is available ...
    saveLocation = {mia::homeLocation(), "Saves/", this->name(), ".", extension, "/", name};
  }
  //try to ensure the directory exists ...
  directory::create(Location::path(saveLocation));
  return saveLocation;
}
