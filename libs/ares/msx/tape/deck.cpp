TapeDeck tapeDeck{"Tape Deck"};

TapeDeck::TapeDeck(string name) : name(name) {
}

auto TapeDeck::load(Node::Object parent) -> void {
  node = parent->append<Node::Peripheral>(name);

  play = node->append<Node::Setting::Boolean>("Playing", false, [&](auto value) {
    if(value == 0) state.playing = 0;
    if(value == 1) state.playing = 1;
  });
  play->setDynamic(true);

  tray.load(node);
}

auto TapeDeck::power() -> void {
  state = {};
}

auto TapeDeck::unload() -> void {
  tray.unload();
  node = {};
}
