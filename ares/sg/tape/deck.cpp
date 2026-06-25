TapeDeck tapeDeck{"Tape Deck"};

TapeDeck::TapeDeck(string name) : name(name) {
}

auto TapeDeck::load(Node::Object parent) -> void {
  node = parent->append<Node::Peripheral>(name);

  tray.load(node);
}

auto TapeDeck::power() -> void {
}

auto TapeDeck::unload() -> void {
  tray.unload();
  node = {};
}
