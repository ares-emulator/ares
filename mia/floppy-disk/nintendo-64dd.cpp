auto Nintendo64DD::export(string location) -> vector<u8> {
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

auto Nintendo64DD::import(string location) -> string {
  auto input = Media::read(location);

  location = {pathname, Location::prefix(location), "/"};
  if(!directory::create(location)) return "output directory not writable";

  file::write({location, "program.disk"}, input);
  return {};
}
