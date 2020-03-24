struct Settings {
  auto serialize() -> string;
  auto unserialize(const string&) -> void;

  boolean createManifests  = false;
  boolean useDatabase      = true;
  boolean useHeuristics    = true;
  string recent            = {Path::user()};
};

extern Settings settings;
