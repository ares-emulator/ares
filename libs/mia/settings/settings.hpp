struct Settings {
  auto serialize() -> string;
  auto unserialize(const string&) -> void;

  Boolean createManifests = false;
  Boolean useDatabase     = true;
  Boolean useHeuristics   = true;
  string recent           = Path::user();
};

extern Settings settings;
