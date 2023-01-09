struct TapeDeckTray {
  Node::Port port;
  Tape tape;

  auto load(Node::Object) -> void;
  auto unload() -> void;
};

extern TapeDeckTray tapeDeckTray;
