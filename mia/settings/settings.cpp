Settings settings;

auto Settings::serialize() -> string {
  string s;
  #define write(name) s.append(#name, ": ", name, "\n")
  write(createManifests);
  write(useDatabase);
  write(useHeuristics);
  write(recent);
  #undef write
  return s;
}

auto Settings::unserialize(const string& markup) -> void {
  *this = {};
  auto s = BML::unserialize(markup);
  #define read(name, type) if(auto node = s[#name]) name = node.type()
  read(createManifests, boolean);
  read(useDatabase, boolean);
  read(useHeuristics, boolean);
  read(recent, string);
  #undef read
}
