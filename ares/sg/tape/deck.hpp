struct TapeDeck {
  Node::Peripheral node;
  TapeDeckTray tray;

  TapeDeck(string name);

  auto playing() -> bool { return tray.tape.node && tray.tape.node->playing(); }
  auto read() -> u1 { return tray.tape.read(); }
  auto write(n1 data) -> void { tray.tape.write(data); }

  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;

  const string name;
};

extern TapeDeck tapeDeck;
