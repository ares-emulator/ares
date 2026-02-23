struct TapeDeck {
  Node::Peripheral node;
  Node::Setting::Boolean play;
  TapeDeckTray tray;

  TapeDeck(string name);

  auto playing() -> bool { return state.playing; }
  auto read() -> u1 { return state.output; }

  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;

  const string name;

  struct {
    bool playing;
    u1 output;
  } state;
};

extern TapeDeck tapeDeck;
