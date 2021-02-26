ArcadeStick::ArcadeStick(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Arcade Stick");
}
