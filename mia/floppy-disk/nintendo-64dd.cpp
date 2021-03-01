auto Nintendo64DD::load(string location) -> shared_pointer<vfs::directory> {
  vector<u8> input;
  if(directory::exists(location)) {
    append(input, {location, "program.disk"});
  } else if(file::exists(location)) {
    input = FloppyDisk::read(location);
  } else {
    return {};
  }

  auto pak = shared_pointer{new vfs::directory};
  pak->append("manifest.bml", heuristics(input, location));
  pak->append("program.disk", input);
  return pak;
}

auto Nintendo64DD::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  return true;
}

auto Nintendo64DD::heuristics(vector<u8>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name: ",  Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  return s;
}
