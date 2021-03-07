struct Nintendo64DD : FloppyDisk {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto extensions() -> vector<string> override { return {"n64dd", "ndd"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto Nintendo64DD::load(string location) -> bool {
  vector<u8> input;
  if(directory::exists(location)) {
    append(input, {location, "program.disk"});
  } else if(file::exists(location)) {
    input = FloppyDisk::read(location);
  }
  if(!input) return false;

  this->location = location;
  this->manifest = analyze(input);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = shared_pointer{new vfs::directory};
  pak->setAttribute("title", document["game/title"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.disk", input);

  return true;
}

auto Nintendo64DD::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto Nintendo64DD::analyze(vector<u8>& rom) -> string {
  string s;
  s += "game\n";
  s +={"  name: ",  Medium::name(location), "\n"};
  s +={"  title: ", Medium::name(location), "\n"};
  return s;
}
