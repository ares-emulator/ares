auto Nintendo64DD::pak(string location) -> shared_pointer<vfs::directory> {
  if(auto pak = Media::pak(location)) return pak;
  if(auto input = Media::read(location)) {
    auto pak = shared_pointer{new vfs::directory};
    pak->append("manifest.bml", heuristics(input, location));
    pak->append("program.disk", input);
    return pak;
  }
  return {};
}

auto Nintendo64DD::rom(string location) -> vector<u8> {
  vector<u8> data;
  append(data, {location, "program.disk"});
  return data;
}

auto Nintendo64DD::heuristics(vector<u8>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name: ",  Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  return s;
}
